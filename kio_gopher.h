/***************************************************************************
 *   Copyright (C) 2003-2008 by Albert Astals Cid                          *
 *   aacid@kde.org                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef __kio_gopher_h__
#define __kio_gopher_h__

#include <kio/tcpslavebase.h>

class gopher : public KIO::TCPSlaveBase
{
	public:
		gopher(const QByteArray &pool_socket, const QByteArray &app_socket);

		void get(const KUrl& url);

	private:
		void processDirectory(QByteArray *received, const QString &host, const QString &path);
		void processDirectoryLine(QByteArray data, QByteArray &show, QByteArray &info);
		QByteArray parsePort(QByteArray *received);
		void findLine(QByteArray *received, int *i, int *remove);
		void handleSearch(const QString &host, const QString &path, int port);
};

#endif

