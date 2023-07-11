/*
 *   SPDX-FileCopyrightText: 2003-2008 Albert Astals Cid <aacid@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kio_gopher.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QFile>
#include <QMimeDatabase>
#include <QMimeType>
#include <klocalizedstring.h>

using namespace KIO;

class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.slave.gopher" FILE "gopher.json")
};

extern "C" {
int Q_DECL_EXPORT kdemain(int argc, char **argv)
{
    QCoreApplication app(argc, argv); // needed for QSocketNotifier
    app.setApplicationName(QLatin1String("kio_gopher"));

    if (argc != 4) {
        fprintf(stderr, "Usage: kio_gopher protocol domain-socket1 domain-socket2\n");
        exit(-1);
    }

    gopher slave(argv[2], argv[3]);
    slave.dispatchLoop();
    return 0;
}
}

/* gopher */

gopher::gopher(const QByteArray &pool_socket, const QByteArray &app_socket)
    : WorkerBase("gopher", pool_socket, app_socket)
{
}

KIO::WorkerResult gopher::get(const QUrl &url)
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
    if (path != "/" && path != "")
        type = path[1];
    else
        type = '1';

    // determine the port
    if (url.port() > 0)
        port = url.port();
    else
        port = 70;

    // connect to the host
    if (auto result = connectToHost("gopher", url.host(), port); !result.success())
        return result;

    if (type == '7' && query.isNull()) {
        disconnectFromHost();
        handleSearch(url.host(), path, port);
    } else {
        int i, bytes;
        char aux[10240];
        QBuffer received;
        received.open(QIODevice::WriteOnly);

        infoMessage(i18n("Connecting to %1...", url.host()));
        infoMessage(i18n("%1 contacted. Retrieving data...", url.host()));
        bytes = 0;

        // send the selector
        path.remove(0, 2);
        socketWrite(path.toLatin1(), path.length());
        socketWrite(query.toLatin1(), query.length());
        socketWrite("\r\n", 2);

        // read the data
        while ((i = socketRead(aux, 10240)) > 0) {
            bytes += i;
            received.write(aux, i);
            processedSize(bytes);
            infoMessage(i18n("Retrieved %1 bytes from %2...", bytes, url.host()));
        }

        if (type == '1' || type == '7')
            processDirectory(new QByteArray(received.buffer().data(), bytes + 1), url.host(), url.path());
        else {
            QMimeDatabase db;
            QMimeType result = db.mimeTypeForData(received.buffer());
            mimeType(result.name());
            data(received.buffer());
        }
        disconnectFromHost();
    }
    return KIO::WorkerResult::pass();
}

void gopher::processDirectory(QByteArray *received, const QString &host, const QString &path)
{
    int i, remove;
    QString pathToShow;
    QByteArray show;
    QByteArray info;
    if (path == "/" || path == "/1")
        pathToShow = "";
    else
        pathToShow = path;
    mimeType("text/html");
    show.append("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n");
    show.append("<html xmlns=\"http://www.w3.org/1999/xhtml\">\n");
    show.append("\t<head>\n");
    show.append("\t\t<title>");
    show.append(host.toUtf8());
    show.append(pathToShow.toUtf8());
    show.append("</title>\n");
    show.append("\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n");
    show.append("\t\t<style type=\"text/css\">\n");
    show.append("\t\t\t.info{ font-size : small; display : block; font-family : monospace; white-space : pre; margin-left : 18px; }\n");
    show.append("\t\t</style>\n");
    show.append("\t</head>\n");
    show.append("\t<body>\n");
    show.append("\t\t<h1>");
    show.append(host.toUtf8());
    show.append(pathToShow.toUtf8());
    show.append("</h1>\n");
    findLine(received, &i, &remove);
    while (i != -1) {
        processDirectoryLine(received->left(i), show, info);
        received->remove(0, i + remove);
        findLine(received, &i, &remove);
    }
    show.append("\t</body>\n");
    show.append("</html>\n");
    data(show);
    delete received;
}

void gopher::processDirectoryLine(const QByteArray &d, QByteArray &show, QByteArray &info)
{
    // gopher <type><display><tab><selector><tab><server><tab><port><\r><\n>
    // gopher+ <type><display><tab><selector><tab><server><tab><port><tab><things><\r><\n>
    int i;
    QByteArray type, name, url, server, port;
    QByteArray data = d;

    type = data.left(1);
    data.remove(0, 1);

    i = data.indexOf("\t");
    name = data.left(i);
    data.remove(0, i + 1);

    i = data.indexOf("\t");
    url = data.left(i);
    data.remove(0, i + 1);

    i = data.indexOf("\t");
    server = data.left(i);
    data.remove(0, i + 1);

    port = parsePort(&data);

    if (type == "i") {
        if (!info.isEmpty())
            info.append("\n");
        info.append(name);
    } else {
        if (!info.isEmpty()) {
            show.append("\t\t<div class=\"info\">");
            show.append(info);
            show.append("</div>\n");
            info = "";
        }
        // it's the final line, ignore it
        if (type == ".")
            return;
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
        show.append("\t\t\t<div>");
        // support the non-standard extension for URL to external sites
        // in this case, url begins with 'URL:'
        QByteArray finalUrl;
        QByteArray iconUrl;
        if (url.startsWith("URL:")) {
            finalUrl = url.mid(4);
            iconUrl = finalUrl;
        } else {
            finalUrl = "gopher://" + server;
            if (port != "70") {
                finalUrl.append(":");
                finalUrl.append(port);
            }
            finalUrl.append('/' + type + url);
            iconUrl = url;
        }
        show.append("\t\t\t\t<a href=\"");
        show.append(finalUrl);
        show.append("\">");
        addIcon(type, iconUrl, show);
        show.append(name);
        show.append("</a><br />\n");
        show.append("\t\t\t</div>");
    }
}

QByteArray gopher::parsePort(QByteArray *received)
{
    int i = 0;
    QByteArray port;
    bool found = false;
    QChar c;
    while (!found && i < received->size()) {
        c = received->at(i);
        if (c.isDigit())
            i++;
        else
            found = true;
    }
    port = received->left(i);
    received->remove(0, i);
    return port;
}

void gopher::findLine(QByteArray *received, int *i, int *remove)
{
    // it's not in the rfc but most servers don't follow the spec
    // find lines ending only in \n and in \r\n
    int aux, aux2;
    aux = received->indexOf("\r\n");
    aux2 = received->indexOf("\n");

    if (aux == -1) {
        *i = aux2;
        *remove = 1;
    } else {
        if (aux2 < aux) {
            *remove = 1;
            *i = aux2;
        } else {
            *remove = 2;
            *i = aux;
        }
    }
}

void gopher::handleSearch(const QString &host, const QString &path, int port)
{
    QByteArray show;
    QString sPort;
    if (port != 70)
        sPort = ':' + QString::number(port);
    mimeType("text/html");
    show.append("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n");
    show.append("<html xmlns=\"http://www.w3.org/1999/xhtml\">\n");
    show.append("\t<head>\n");
    show.append("\t\t<title>");
    show.append(host.toUtf8());
    show.append(path.toUtf8());
    show.append("</title>\n");
    show.append("\t\t<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n");
    show.append("\t\t<script type=\"text/javascript\">\n");
    show.append("\t\t\tfunction search()\n");
    show.append("\t\t\t{\n");
    show.append("\t\t\t\tdocument.location = 'gopher://");
    show.append(host.toUtf8());
    show.append(sPort.toUtf8());
    show.append(path.toUtf8());
    show.append("?' + document.getElementById('what').value;\n");
    show.append("\t\t\t}\n");
    show.append("\t\t</script>\n");
    show.append("\t</head>\n");
    show.append("\t<body>\n");
    show.append("\t\t<h1>");
    show.append(host.toUtf8());
    show.append(path.toUtf8());
    show.append("</h1>\n");
    show.append("\t\t");
    show.append(i18n("Enter a search term:").toUtf8());
    show.append("<br />\n");
    show.append("\t\t<input id=\"what\" type=\"text\">\n");
    show.append("\t\t<input type=\"button\" value=\"");
    show.append(i18nc("Text on a search button, like at a search engine", "Search").toUtf8());
    show.append("\" onClick=\"search()\">\n");
    show.append("\t</body>\n");
    show.append("</html>\n");
    data(show);
}

void gopher::addIcon(const QString &type, const QByteArray &url, QByteArray &show)
{
    QString icon;
    QMimeDatabase db;
    if (type == "1")
        icon = "inode-directory";
    else if (type == "3")
        icon = "dialog-error";
    else if (type == "7")
        icon = "system-search";
    else if (type == "g")
        icon = "image-gif";
    else if (type == "I")
        icon = "image-x-generic";
    else {
        QMimeType mime = db.mimeTypeForFile(QUrl(url).path(), QMimeDatabase::MatchExtension);
        icon = mime.iconName();
    }
    QFile file(m_iconLoader.iconPath(icon, -16));
    file.open(QIODevice::ReadOnly);
    const QMimeType iconMime = db.mimeTypeForFile(file.fileName(), QMimeDatabase::MatchExtension);
    QByteArray ba = file.readAll();
    show.append("<img width=\"16\" height=\"16\" src=\"data:");
    show.append(iconMime.name().toLatin1());
    show.append(";base64,");
    show.append(ba.toBase64());
    show.append("\" /> ");
}

KIO::WorkerResult gopher::connectToHost(const QString & /*protocol*/, const QString &host, quint16 port)
{
    QString errorString;
    const int errCode = connectToHost(host, port, &errorString);
    if (errCode == 0) {
        return WorkerResult::pass();
    }

    return WorkerResult::fail(errCode, errorString);
}

int gopher::connectToHost(const QString &host, quint16 port, QString *errorString)
{
    if (errorString) {
        errorString->clear(); // clear prior error messages.
    }

    const int timeout = (connectTimeout() * 1000); // 20 sec timeout value

    disconnectFromHost(); // Reset some state, even if we are already disconnected

    socket.connectToHost(host, port);
    socket.waitForConnected(timeout > -1 ? timeout : -1);

    if (socket.state() != QAbstractSocket::ConnectedState) {
        if (errorString) {
            *errorString = host + QLatin1String(": ") + socket.errorString();
        }
        switch (socket.error()) {
        case QAbstractSocket::UnsupportedSocketOperationError:
            return ERR_UNSUPPORTED_ACTION;
        case QAbstractSocket::RemoteHostClosedError:
            return ERR_CONNECTION_BROKEN;
        case QAbstractSocket::SocketTimeoutError:
            return ERR_SERVER_TIMEOUT;
        case QAbstractSocket::HostNotFoundError:
            return ERR_UNKNOWN_HOST;
        default:
            return ERR_CANNOT_CONNECT;
        }
    }

    return 0;
}

void gopher::disconnectFromHost()
{
    if (socket.state() == QAbstractSocket::UnconnectedState) {
        // discard incoming data - the remote host might have disconnected us in the meantime
        // but the visible effect of disconnectFromHost() should stay the same.
        socket.close();
        return;
    }

    socket.disconnectFromHost();
    if (socket.state() != QAbstractSocket::UnconnectedState) {
        socket.waitForDisconnected(-1); // wait for unsent data to be sent
    }
    socket.close(); // whatever that means on a socket
}

ssize_t gopher::socketWrite(const char *data, ssize_t len)
{
    ssize_t written = socket.write(data, len);

    bool success = socket.waitForBytesWritten(-1);

    socket.flush(); // this is supposed to get the data on the wire faster

    if (socket.state() != QAbstractSocket::ConnectedState || !success) {
        return -1;
    }

    return written;
}

ssize_t gopher::socketRead(char *data, ssize_t len)
{
    if (!socket.bytesAvailable()) {
        socket.waitForReadyRead(-1);
    }
    return socket.read(data, len);
}

#include "kio_gopher.moc"
