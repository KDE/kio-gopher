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

#include <qcstring.h>
#include <qdict.h>
#include <qstring.h>

#include <kurl.h>
#include <kio/tcpslavebase.h>

class entryInfo
{
	public:
		entryInfo(QString display, QString selector, QString host, int port, bool isPlus);
		
	private:
		QString m_display;
		QString m_selector;
		QString m_host;
		int m_port;
		bool m_isPlus;
};

class gopher : public KIO::TCPSlaveBase
{
	public:
		gopher(const QCString &pool_socket, const QCString &app_socket);
		virtual ~gopher();

		virtual void setHost (const QString &host, int port, const QString &user, const QString &pass);
		virtual void get(const KURL& url);

	private:
		void processDirectory(QCString *received, QString host, QString path);
		void processDirectoryLine(QCString data, QCString *show, QString *info);
		bool seemsDirectory(QCString *received);
		bool seemsDirectoryLine(QCString received);
		int parsePort(QCString received);
		
		QDict<entryInfo> lastDir;
};

#endif

