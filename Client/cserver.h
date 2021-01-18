#ifndef CSERVER_H
#define CSERVER_H

#include <QtWidgets>
#include <QtNetwork>
#include <QDebug>
#include <QRegExp>

#include "src/includes/AbstractServer.h"

#include "src/includes/cclient.h"
#include "src/includes/cchannel.h"
#include "src/includes/audioinput.h"
#include "src/includes/audiooutput.h"



class CClient;
class CChannel;

class CServer : public AbstractServer
{
    public:

    CServer();

    //Getters
    QTcpSocket * get_socket();

    //Setters
    void set_socket(QTcpSocket* soc);

    //Server action - To develop
    void changeChannel(int id);
    void quitChannel(int id);
    void joinChannel(int id);

        //Serialization | Deserialization
        QByteArray Serialize();                                             //Serialize client and channels on the same json document
        void Deserialize(QByteArray in);

        QByteArray SerializeChannels();                                     //Serialize channels into json document then byte array
        QByteArray SerializeClients();                                      //Serialize clients into json document then byte array

        void DeserializeChannels(QByteArray in);                            //Deserialize channels from byte array
        void DeserializeClient(QByteArray in);                              //Deserialize clients from byte array

        void deserializeChannel(QJsonArray & json_array);
        void deserializeClients(QJsonArray & json_array);

        CChannel * deserializeToChannel(QJsonObject json_obj);              //Deserialize channels from json object
        CClient * deserializeToClient(QJsonObject json_obj);                //Deserialize clients from json object


public slots:

        void sendToAll(QByteArray out);

    private slots:

        void onReceiveData();

        void sendToServer(QByteArray ba);
        void sendToServer();

    private:
        //Client mode
        QTcpSocket * m_socket;

};

#endif // CSERVER_H
