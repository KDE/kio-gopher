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

#include <stdlib.h>

#include "kio_gopher.h"

using namespace KIO;

static const QString defaultRefreshRate = "60";

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
	char aux[10241];
	QCString *received = new QCString();
	
	if (url.port() != 0) port = url.port();
	else port = 70;
	infoMessage(i18n("Connecting to %1...").arg(url.host()));
	if (!connectToHost(url.host(), port)) return;	
	infoMessage(i18n("%1 contacted. Retrieving data...").arg(url.host()));
	bytes = 0;

	// if the user is not asking for the server root send what he is asking for
	if (url.path() != "") write(url.path().latin1(), url.path().length());
	write("\r\n", 2);
	while((i = read(aux, 1024)) > 0)
	{
		bytes += i;
		aux[i] = '\0';
		received -> append(aux);
		processedSize(bytes);
		infoMessage(i18n("Retrieved %1 bytes from %2...").arg(bytes).arg(url.host()));
	}
	if (seemsDirectory(received)) processDirectory(received, url.host(), url.path());
	else
	{
		// if it is not a directoy be dump the data completely on konqy
		// hoping it'll not how to handle it
		// TODO: use kmimetype guessing habilities
		data(*received);
	}
	closeDescriptor();
    finished();
}

void gopher::processDirectory(QCString *received, QString host, QString path)
{
	QCString *show = new QCString();
	mimeType("text/html");
	if (path != "" && path.left(1) != "/") path.prepend("/");
	show -> append("<html>\n\t<head>\n\t\t<title>" + host + path + "</title>\n\t</head>\n");
	show -> append("\t<body>\n\t\t<h1>" + host + path + "</h1>\n");
	int i = received -> find("\r\n");
	while(i != -1)
	{
		processDirectoryLine(received -> left(i), show);
		received -> remove(0, i + 2);
		i = received -> find("\r\n");
	}
	show -> append("\t</body>\n</html>");
	data(*show);	
}

void gopher::processDirectoryLine(QCString data, QCString *show)
{
	int i;
	QString c, name, url, server, port;
	c = data.left(1);
	data.remove(0, 1);
	if (c == "0" || c == "1")
	{
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
		show -> append("\t\t<a href=\"gopher://");
		show -> append(server);
		show -> append(url);
		show -> append("\">");
		show -> append(name);
		show -> append("</a>\n");
		if (port != "70")
		{
			show -> append(":");
			show -> append(port);
		}
		show -> append("<br>\n");
	}
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
	int i;
	i = received.find("\t");
	received.remove(0, i + 1);
	i = received.find("\t");
	received.remove(0, i + 1);
	i = received.find("\t");
	if (i == -1) seems = false;
	else
	{
		// do more things
		seems = true;
	}
	return seems;
}

