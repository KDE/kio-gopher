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

entryInfo::entryInfo(QString display, QString selector, QString host, int port, bool isPlus)
{
	m_display = display;
	m_selector = selector;
	m_host = host;
	m_port = port;
	m_isPlus = isPlus;
}

/* gopher */

gopher::gopher(const QCString &pool_socket, const QCString &app_socket) : TCPSlaveBase(70, "gopher", pool_socket, app_socket)
{
}


gopher::~gopher()
{
}

void gopher::setHost(const QString &/*host*/, int /*port*/, const QString &/*user*/, const QString &/*pass*/)
{
	// don't do anything but necessary to make it listen for gopher:// instead of gopher:/
	// i think
}

void gopher::get(const KURL& url )
{
	int i, port, bytes;
	char aux[10240];
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
	if (path != "")
	{
		// remove the first / it is difficult to know if it was part of the selector string
		// or not, but server that have selector strings with /foo also
		// return the data with foo and servers with selector string foo don't return 
		// data with /foo so it's better to remove the first /
		path.remove(0, 1);
		write(path.latin1(), path.length());
	}
	write("\r\n", 2);
	while((i = read(aux, 10240)) > 0)
	{
		bytes += i;
		ts.writeRawBytes(aux, i);
		processedSize(bytes);
		infoMessage(i18n("Retrieved %1 bytes from %2...").arg(bytes).arg(url.host()));
	}
	QCString *receivedString = new QCString(received->data());
	if (seemsDirectory(receivedString)) processDirectory(receivedString, url.host(), url.path());
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
	QCString *show = new QCString();
	if (path == "/") path = "";
	mimeType("text/html");
	show -> append("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n");
	show -> append("<html xmlns=\"http://www.w3.org/1999/xhtml\">\n\t<head>\n\t\t<title>" + host + path + "</title>\n\t</head>\n");
	show -> append("\t<body>\n\t\t<h1>" + host + path + "</h1>\n\t\t<ul>\n");
	int i = received -> find("\r\n");
	while(i != -1)
	{
		processDirectoryLine(received -> left(i), show);
		received -> remove(0, i + 2);
		i = received -> find("\r\n");
	}
	show -> append("\t\t</ul>\n\t</body>\n</html>");
	data(*show);	
}

void gopher::processDirectoryLine(QCString data, QCString *show)
{
	// gopher <type><display><tab><selector><tab><server><tab><port><\r><\n>
	// gopher+ <type><display><tab><selector><tab><server><tab><port><tab><things><\r><\n>
	// TODO: use parsePort to obtain the port
	int i;
	QString type, name, url, server, port;
	type = data.left(1);
	data.remove(0, 1);
	// Right now don't use types for nothing
	// those are the standard gopher types
	//if (type == "0" || type == "1" || type == "2" || type == "4" || type == "5" 
	//	|| type == "6" || type == "7" || type == "8" || type == "9" || type == "+" 
	//	|| type == "T" || type == "g" || type == "I")
//	{
		i = data.find("\t");
		name = data.left(i);
		data.remove(0, i + 1);
		i = data.find("\t");
		url = data.left(i);
		data.remove(0, i + 1);
		i = data.find("\t");
		server = data.left(i);
		data.remove(0, i + 1);
		i = data.find("\t");
		port = data.left(i);
		data.remove(0, i + 1);
		
		if (url.left(1) != "/") url.prepend("/");
		show -> append("\t\t\t<li>\n\t\t\t\t<a href=\"gopher://");
		show -> append(server);
		if (port != "70")
		{
			show -> append(":");
			show -> append(port);
		}
		show -> append(url);
		show -> append("\">");
		show -> append(name);
		show -> append("</a>\n\t\t\t</li>\n");
//	}
}

bool gopher::seemsDirectory(QCString *received)
{
	// RFC says directorys SHOULD end with a point but some do not
	int i;
	bool endingPointFound = false;
	QCString copy(*received);
	bool seems = true;
	
	i = copy.find("\r\n");
	if (i == -1) seems = false;	
	while (seems && i != -1 && !endingPointFound)
	{
		if (copy.left(i) == ".") endingPointFound = true;
		else seems = seemsDirectoryLine(copy.left(i));
		copy.remove(0, i + 2);
		i = copy.find("\r\n");
	}
	if (endingPointFound)
	{
		if (i >= 0) seems = false;
	}
	return seems;
}

bool gopher::seemsDirectoryLine(QCString received)
{
	bool seems;
	int i, port;

	// gopher <type><display><tab><selector><tab><server><tab><port><\r><\n>
	// gopher+ <type><display><tab><selector><tab><server><tab><port><tab><things><\r><\n>
	
	i = received.find("\t");
	received.remove(0, i + 1); //remove type and display
	
	i = received.find("\t");
	received.remove(0, i + 1); //remove selector
	
	i = received.find("\t");
	received.remove(0, i + 1); //remove server
	
	port = parsePort(received);
	
	if (port == -1 || i == -1) seems = false;
	else
	{
		// do more things
		seems = true;
	}
	return seems;
}

int gopher::parsePort(QCString received)
{
	uint i = 0;
	bool found = false;
	QChar c;
	while (!found && i < received.size())
	{
		c = received[i];
		if (c.isDigit()) i++;
		else found = true;
	}
	if (i == 0) return -1;
	else return received.left(i).toInt();
}

