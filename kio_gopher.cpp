/***************************************************************************
 **   Copyright (C) 2003 by Albert Astals Cid                               *
 **   tsdgeos@terra.es                                                      *
 **                                                                         *
 **   This program is free software; you can redistribute it and/or modify  *
 **   it under the terms of the GNU General Public License as published by  *
 **   the Free Software Foundation; either version 2 of the License, or     *
 **   (at your option) any later version.                                   *
 ****************************************************************************/

#include <kiconloader.h>
#include <kinstance.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmimetype.h>

#include <stdlib.h>

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

/* entryInfo */

/*entryInfo::entryInfo(QString display, QString selector, QString host, int port, bool isPlus)
{
	m_display = display;
	m_selector = selector;
	m_host = host;
	m_port = port;
	m_isPlus = isPlus;
}*/

/* gopher */

gopher::gopher(const QCString &pool_socket, const QCString &app_socket) : TCPSlaveBase(70, "gopher", pool_socket, app_socket)
{
}


gopher::~gopher()
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
	int i, port, bytes;
	char aux[10240];
	QChar type;
	QString path(url.path());
	QByteArray *received = new QByteArray();
	QTextStream ts(*received, IO_WriteOnly);
	
	if (url.port() != 0) port = url.port();
	else port = 70;
	infoMessage(i18n("Connecting to %1...").arg(url.host()));
	if (!connectToHost(url.host(), port)) return;	
	infoMessage(i18n("%1 contacted. Retrieving data...").arg(url.host()));
	bytes = 0;

	// if the user is not asking for the server root send what he is asking for	
	if (path != "/" && path != "" && path != "/1")
	{
		type = path[1];
		path.remove(0, 2);
		write(path.latin1(), path.length());
	}
	else type = '1';
	write("\r\n", 2);
	while((i = read(aux, 10240)) > 0)
	{
		bytes += i;
		ts.writeRawBytes(aux, i);
		processedSize(bytes);
		infoMessage(i18n("Retrieved %1 bytes from %2...").arg(bytes).arg(url.host()));
	}
	if (type == '1') processDirectory(new QCString(received -> data()), url.host(), url.path());
	else
	{
		// we pass all the received data to kmimetype and hope we get the good mimetype
		// hoping konqueror will know how to handle it		
		mimeType(KMimeType::findByContent(*received)->name());
		data(*received);
	}
	closeDescriptor();
	finished();
}

void gopher::processDirectory(QCString *received, QString host, QString path)
{
	// TODO Make the <li> icon thing work
	QCString *show = new QCString();
	QString folderPath, *info = new QString();
	folderPath = KGlobal::iconLoader() -> iconPath("folder", 0);
	if (path == "/") path = "";
	mimeType("text/html");
	show -> append("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n");
	show -> append("<html xmlns=\"http://www.w3.org/1999/xhtml\">\n\t<head>\n\t\t<title>" + host + path + "</title>\n\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=ISO-8859-1\" />\n\t\t<style type=\"text/css\">\n\t\t\t.information{ border : 1px solid #000000; text-align : center; background-color : #abcdef; }\n\t\t\t.folder{ list-style-image: url(" + folderPath + ") }\n\t\t</style>\n\t</head>\n");
	show -> append("\t<body>\n\t\t<h1>" + host + path + "</h1>\n\t\t<ul>\n");
	int i = received -> find("\r\n");
	while(i != -1)
	{
		processDirectoryLine(received -> left(i), show, info);
		received -> remove(0, i + 2);
		i = received -> find("\r\n");
	}
	show -> append("\t\t</ul>\n");
	if (!info -> isEmpty()) show -> append("\t\t<table width=\"75%\" class=\"information\">\n\t\t\t<caption>" + i18n("Information") + "</caption>\n\t\t\t<tr>\n\t\t\t\t<td>" + *info + "</td>\n\t\t\t</tr>\n\t\t</table>\n");
	show -> append("\t</body>\n</html>");
	data(*show);	
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
	}
	else if (type == ".")
	{
		// it's final line, ignore it
	}
	else
	{
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
		if (type == "1") show -> append("\t\t\t<li class=\"folder\">\n\t\t\t\t<a href=\"gopher://");
		else show -> append("\t\t\t<li>\n\t\t\t\t<a href=\"gopher://");

		show -> append(server);
		if (port != "70")
		{
			show -> append(":");
			show -> append(port);
		}
		show -> append("/" + type + url);
		show -> append("\">");
		show -> append(name);
		show -> append("</a>\n\t\t\t</li>\n");
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
