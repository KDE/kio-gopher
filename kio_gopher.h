/***************************************************************************
 **   Copyright (C) 2003 by Albert Astals Cid                               *
 **   tsdgeos@terra.es                                                      *
 **                                                                         *
 **   This program is free software; you can redistribute it and/or modify  *
 **   it under the terms of the GNU General Public License as published by  *
 **   the Free Software Foundation; either version 2 of the License, or     *
 **   (at your option) any later version.                                   *
 ****************************************************************************/

#ifndef __kio_gopher_h__
#define __kio_gopher_h__

#include <kio/tcpslavebase.h>

class gopher : public KIO::TCPSlaveBase
{
	public:
		gopher(const QCString &pool_socket, const QCString &app_socket);

		void get(const KURL& url);

	private:
		void processDirectory(QCString *received, QString host, QString path);
		void processDirectoryLine(QCString data, QCString *show, QString *info);
		QString parsePort(QCString *received);
		void findLine(QCString *received, int *i, int *remove);
		void handleSearch(QString host, QString path, int port);
};

#endif

