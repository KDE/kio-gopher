/***************************************************************************
 **   Copyright (C) 2003 by Albert Astals Cid                               *
 **   tsdgeos@terra.es                                                      *
 **                                                                         *
 **   This program is free software; you can redistribute it and/or modify  *
 **   it under the terms of the GNU General Public License as published by  *
 **   the Free Software Foundation; either version 2 of the License, or     *
 **   (at your option) any later version.                                   *
 ****************************************************************************/

#include <kinstance.h>
#include <klocale.h>
#include <kmimemagic.h>

#include "kio_gopher.h"

using namespace KIO;

extern "C"
{
	int kdemain( int argc, char **argv )
	{
		KInstance instance( "kio_gopher" );

		if (argc != 4)
		{
			fprintf(stderr, "Usage: kio_gopher protocol domain-socket1 domain-socket2\n");
			exit(-1);
		}

		gopher slave(argv[2], argv[3]);
		slave.dispatchLoop();
		return 0;
	}
}

/* gopher */

gopher::gopher(const QCString &pool_socket, const QCString &app_socket) : TCPSlaveBase(70, "gopher", pool_socket, app_socket)
{
}

void gopher::get(const KURL& url )
{
	// gopher urls are
	// gopher://<host>:<port>/<gopher-path>
	//
	//  where <gopher-path> is one of
	//
	//  <gophertype><selector>
	//  <gophertype><selector>%09<search>
	//  <gophertype><selector>%09<search>%09<gopher+_string>
	int port;
	QChar type;
	QString path(url.path());
	QString query(url.query());

	// determine the type
	if (path != "/" && path != "") type = path[1];
	else type = '1';
	
	// determine the port
	if (url.port() != 0) port = url.port();
	else port = 70;
	
	if (type == '7' && query.isNull())
	{
		handleSearch(url.host(), path, port);
	}
	else
	{
		int i, bytes;
		char aux[10240];
		QByteArray *received = new QByteArray();
		QDataStream stream(*received, IO_WriteOnly);
		
		infoMessage(i18n("Connecting to %1...").arg(url.host()));
		if (!connectToHost(url.host(), port)) return;
		infoMessage(i18n("%1 contacted. Retrieving data...").arg(url.host()));
		bytes = 0;

		// send the selector
		path.remove(0, 2);
		write(path.latin1(), path.length());
		write(query.latin1(), query.length());
		write("\r\n", 2);

		// read the data
		while((i = read(aux, 10240)) > 0)
		{
			bytes += i;
			stream.writeRawBytes(aux, i);
			processedSize(bytes);
			infoMessage(i18n("Retrieved %1 bytes from %2...").arg(bytes).arg(url.host()));
		}

		if (type == '1' || type =='7') processDirectory(new QCString(received -> data(), bytes + 1), url.host(), url.path());
		else
		{
			KMimeMagicResult *result = KMimeMagic::self() -> findBufferType(*received);
			mimeType(result->mimeType());
			data(*received);
		}
		closeDescriptor();
		delete received;
	}
	finished();
}

void gopher::processDirectory(QCString *received, QString host, QString path)
{
	int i, remove;
	QCString *show = new QCString();
	QString *info = new QString();
	if (path == "/" || path == "/1") path = "";
	mimeType("text/html");
	show -> append("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n");
	show -> append("<html xmlns=\"http://www.w3.org/1999/xhtml\">\n");
	show -> append("\t<head>\n");
	show -> append("\t\t<title>" + host + path + "</title>\n");
	show -> append("\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=ISO-8859-1\" />\n");
	show -> append("\t\t<style type=\"text/css\">\n");
	show -> append("\t\t\t.info{ font-size : small; }\n");
	show -> append("\t\t</style>\n");
	show -> append("\t</head>\n");
	show -> append("\t<body>\n");
	show -> append("\t\t<h1>" + host + path + "</h1>\n");
	findLine(received, &i, &remove);
	while(i != -1)
	{
		processDirectoryLine(received -> left(i), show, info);
		received -> remove(0, i + remove);
		findLine(received, &i, &remove);
	}
	show -> append("\t</body>\n");
	show -> append("</html>\n");
	data(*show);
	delete show;
	delete info;
	delete received;
}

void gopher::processDirectoryLine(QCString data, QCString *show, QString *info)
{
	// gopher <type><display><tab><selector><tab><server><tab><port><\r><\n>
	// gopher+ <type><display><tab><selector><tab><server><tab><port><tab><things><\r><\n>
	int i;
	QString type, name, url, server, port;
	
	type = data.left(1);
	data.remove(0, 1);
	
	i = data.find("\t");
	name = data.left(i);
	data.remove(0, i + 1);

	i = data.find("\t");
	url = data.left(i);
	data.remove(0, i + 1);
	
	i = data.find("\t");
	server = data.left(i);
	data.remove(0, i + 1);
	
	port = parsePort(&data);
		
	if (type == "i")
	{
		info -> append(name);
		info -> append("<br />");
	}
	else
	{
		if (!info -> isEmpty())
		{
			show -> append("\t\t<div class=\"info\">\n");
			show -> append("\t\t\t" + *info + "\n");
			show -> append("\t\t</div>\n");
			*info = "";
		}
		// it's the final line, ignore it
		if (type == ".") return;
		// those are the standard gopher types defined in the rfc
		//  0   Item is a file
		//  1   Item is a directory
		//  2   Item is a CSO phone-book server
		//  3   Error
		//  4   Item is a BinHexed Macintosh file.
		//  5   Item is DOS binary archive of some sort. Client must read until the TCP connection closes.  Beware.
		//  6   Item is a UNIX uuencoded file.
		//  7   Item is an Index-Search server.
		//  8   Item points to a text-based telnet session.
		//  9   Item is a binary file! Client must read until the TCP connection closes.  Beware.
		//  +   Item is a redundant server
		//  T   Item points to a text-based tn3270 session.
		//  g   Item is a GIF format graphics file.
		//  I   Item is some kind of image file.  Client decides how to display.
		show -> append("\t\t\t<div>");
		show -> append("\t\t\t\t<a href=\"gopher://");
		show -> append(server);
		if (port != "70")
		{
			show -> append(":");
			show -> append(port);
		}
		show -> append("/" + type + url);
		show -> append("\">");
		show -> append("- ");
		show -> append(name);
		show -> append("</a><br />\n");
		show -> append("\t\t\t</div>");
	}
}

QString gopher::parsePort(QCString *received)
{
	uint i = 0;
	QString port;
	bool found = false;
	QChar c;
	while (!found && i < received -> size())
	{
		c = received -> at(i);
		if (c.isDigit()) i++;
		else found = true;
	}
	port = received -> left(i);
	received -> remove(0, i);
	return port;
}

void gopher::findLine(QCString *received, int *i, int *remove)
{
	// it's not in the rfc but most servers don't follow the spec
	// find lines ending only in \n and in \r\n
	int aux, aux2;
	aux = received -> find("\r\n");
	aux2 = received -> find("\n");

	if (aux == -1)
	{
		*i = aux2;
		*remove = 1;
	}
	else
	{
		if (aux2 < aux)
		{
			*remove = 1;
			*i = aux2;
		}
		else
		{
			*remove = 2;
			*i = aux;
		}
	}
}

void gopher::handleSearch(QString host, QString path, int port)
{
	QCString *show = new QCString();
	QString sPort;
	if (port != 70) sPort = ":" + QString::number(port);
	mimeType("text/html");
	show -> append("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n");
	show -> append("<html xmlns=\"http://www.w3.org/1999/xhtml\">\n");
	show -> append("\t<head>\n");
	show -> append("\t\t<title>" + host + path + "</title>\n");
	show -> append("\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=ISO-8859-1\" />\n");
	show -> append("\t\t<script type=\"text/javascript\">\n");
	show -> append("\t\t\tfunction search()\n");
	show -> append("\t\t\t{\n");
	show -> append("\t\t\t\tdocument.location = 'gopher://" + host  + sPort + path + "?' + document.getElementById('what').value;\n");
	show -> append("\t\t\t}\n");
	show -> append("\t\t</script>\n");
	show -> append("\t</head>\n");
	show -> append("\t<body>\n");
	show -> append("\t\t<h1>" + host + path + "</h1>\n");
	show -> append("\t\t" + i18n("Enter a search term:") + "<br />\n");
	show -> append("\t\t<input id=\"what\" type=\"text\">\n");
	show -> append("\t\t<input type=\"button\" value=\"" + i18n("Search") + "\" onClick=\"search()\">\n");
	show -> append("\t</body>\n");
	show -> append("</html>\n");
	data(*show);
	delete show;
}
