/*
 *   SPDX-FileCopyrightText: 2003-2008 Albert Astals Cid <aacid@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __kio_gopher_h__
#define __kio_gopher_h__

#include <KIO/WorkerBase>
#include <kiconloader.h>

#include <QTcpSocket>

class gopher : public KIO::WorkerBase
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
    KIO::WorkerResult connectToHost(const QString &protocol, const QString &host, quint16 port);
    int connectToHost(const QString &host, quint16 port, QString *errorString = nullptr);
    void disconnectFromHost();
    ssize_t socketWrite(const char *data, ssize_t len);
    ssize_t socketRead(char *data, ssize_t len);

    QTcpSocket socket;
    KIconLoader m_iconLoader;
};

#endif
