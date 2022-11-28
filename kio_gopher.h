/*
 *   SPDX-FileCopyrightText: 2003-2008 Albert Astals Cid <aacid@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __kio_gopher_h__
#define __kio_gopher_h__

#include <kiconloader.h>
#include <kio/tcpworkerbase.h>

class gopher : public KIO::TCPWorkerBase
{
public:
    gopher(const QByteArray &pool_socket, const QByteArray &app_socket);

    KIO::WorkerResult get(const QUrl &url) override;

private:
    void processDirectory(QByteArray *received, const QString &host, const QString &path);
    void processDirectoryLine(const QByteArray &data, QByteArray &show, QByteArray &info);
    QByteArray parsePort(QByteArray *received);
    void findLine(QByteArray *received, int *i, int *remove);
    void handleSearch(const QString &host, const QString &path, int port);
    void addIcon(const QString &type, const QByteArray &url, QByteArray &show);

    KIconLoader m_iconLoader;
};

#endif
