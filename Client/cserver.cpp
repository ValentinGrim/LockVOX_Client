#include "Client/cserver.h"
#include "mainwindow.h"

CServer::CServer()
{
            qDebug() << "Starting LockVox client ! Welcome !" << Qt::endl;
            m_socket = new QTcpSocket();
            m_self = NULL;
            m_socket->abort();
            m_socket->connectToHost("192.168.1.80", 50885);

            connect(m_socket, SIGNAL(readyRead()), this, SLOT(onReceiveData()));
}

//Getters
QTcpSocket * CServer::get_socket(){
    return m_socket;
}

//Setters
void CServer::set_socket(QTcpSocket* soc){
    m_socket  = soc;
}

//Envoie de l'audio au server grâce à un QByteArray
void CServer::sendToServer(QByteArray ba)
{
    //qDebug() << "Data has been send to Server ";
    m_socket->write(ba);
    m_socket->waitForBytesWritten();
}

void CServer::sendToServer()
{

}

void CServer::onReceiveData(){
    // On détermine quel client envoie le message (recherche du QTcpSocket du client)

    QByteArray *data = new QByteArray();
    data->append(m_socket->readAll());

    //Process data
    processIncomingData(*data);
}

void CServer::processIncomingData(QByteArray data){

    CPacket * packet = new CPacket(data,NULL);

    if(packet->GetAction().toInt() == -1 && packet->GetType().toInt() == -1)
    {
       Deserialize(data);
       emit(changeState(1));
    }

    //Récupération du type
    if(packet->GetType().toInt() == 0){
         switch (packet->GetAction().toInt()){
            case 0:
            {
                //New User is now online
                CClient * client = new CClient();
                client = packet->Deserialize_newClient();

                for(int i = 0; i < get_clientList().size(); i++)
                {
                    if(get_clientList()[i]->get_uuid() == client->get_uuid())
                    {
                        get_clientById(client->get_uuid())->set_isOnline(true);
                    }
                }
                free(client);

                break;
            }
            case 1:
            {
                //User is now offline
                CClient * client = packet->Deserialize_newClient();

                for(int i = 0; i < get_clientList().size(); i++){
                    if(get_clientList()[i]->get_uuid() == client->get_uuid()){
                        get_clientById(client->get_uuid())->set_isOnline(false);
                    }
                }
                free(client);

                break;
            }
            case 2:
            {
                //PSEUDO UPDATE
                CClient * c = packet->Deserialize_newClient();
                CClient * client = get_clientById(c->get_uuid());
                client->set_pseudo(c->get_pseudo());
                break;
            }
            case 3:
            {
                //BIO UPDATE
                break;
            }
            case 4:
                //BAN USER
                break;
            case 5:{
                //BAN IP
                //Rajouter système de gestion du temps
                break;
                }
            case 6: {
                //Kick user

                break;
                }
            case 7:
            {
                m_self = packet->Deserialize_authAns();
                if(m_self)
                {
                    qDebug() << "Your UUID is :" << m_self->get_uuid().toString() << Qt::endl;
                    emit(on_Authentification(1));
                }
            }
             case 8: {
                int code = packet->Deserialize_regAns();

                //Register successfully
                if(code == 1){
                    m_self = packet->Deserialize_myClient();

                    if(m_self)
                    {
                        emit(on_Authentification(1));
                    }
                }
             }
        }
    }

    if(packet->GetType().toInt() == 1){
        switch (packet->GetAction().toInt())
        {
                case 0: {
                    //CONNECT CHAN
                    packet->Deserialize_ID();

                    CClient * client = get_clientById(packet->get_IdClient());
                    CChannel * channel = get_channelById(packet->get_IdChannel());

                    if(channel && client){
                        client->set_idChannel(channel->get_id());
                        channel->addUser(client);
                    }

                    qDebug() << client->get_pseudo() << " has join channel " << channel->get_name();
                    break;
                }
                case 1: {
                    //QUIT CHAN
                    packet->Deserialize_ID();

                    CClient * client = get_clientById(packet->get_IdClient());
                    CChannel * channel = get_channelById(packet->get_IdChannel());

                    if(channel && client){
                        client->set_idChannel(-1);
                        channel->delUser(client->get_uuid());
                    }
                    break;
                }
                case 5: {
                    //Create chan voc
                    CChannel * c = packet->Deserialize_newChannel();
                    addChannel(c);

                    break;
                }
                case 6: {
                    //Delete chan voc
                    CChannel * c = packet->Deserialize_newChannel();
                    CChannel * toDelChannel = get_channelById(c->get_id());
                    DelChannel(toDelChannel);
                    break;
                }
                case 7: {
                    //Rename chan voc
                    CChannel * c = packet->Deserialize_newChannel();

                    CChannel * channel = get_channelById(c->get_id());
                    channel->set_name(c->get_name());
                    break;
                }
                case 8: {
                    //Modif max user (voc)
                   CChannel * c = packet->Deserialize_newChannel();
                   CChannel * channel = get_channelById(c->get_id());
                 channel->set_maxUsers(c->get_maxUsers());
                    break;
                }
                case 9: {
                    //kick user voc

                    break;
                }
                case 10: {
                    //Mute user voc (server side)

                    break;
                }
                case 11:{
                    //Create chan text --------> Qxmpp

                    break;
                }
                case 12:
                    //Delete cahn text
                    break;
                case 13:
                    //Rename chan text
                    break;
                default:
                    qDebug() << "Error invalid action" << Qt::endl;
                    break;
            break;
        }
    }

    if(packet->GetType().toInt() == 2){
        switch (packet->GetAction().toInt())
        {
        case 0:
            //Mute (user side) ?????
            break;
        case 1:
            //Add friend --> later
            break;
        case 2:
            //Del friend
            break;
        case 3:
            //Send msg to friend
            break;
        case 4:
            //Modif pseudo (update bdd)
            break;
        case 5:
            //Change right
            break;
        default:
            qDebug() << "Error invalid action" << Qt::endl;
        }
     }
    emit(updateMainWindow());
}

bool CServer::Register(QString username, QString mail, QString password,QString password_confirm)
{
    CPacket reg_pkt("0", "8");

    reg_pkt.Serialize_regReq(username, mail, password, password_confirm);

    if(m_socket->write(reg_pkt.GetByteArray()) == -1)
    {
        qDebug() << "Error in Register, can't write to socket" << Qt::endl;
        return false;
    }
    m_socket->waitForBytesWritten();
    return true;

}

bool CServer::Login(QString mail, QString passwd)
{
    CPacket auth_pkt("0", "7");
    auth_pkt.Serialize_authReq(mail, passwd);
    if(m_socket->write(auth_pkt.GetByteArray()) == -1)
    {
        qDebug() << "Error in Login, can't write to socket" << Qt::endl;
        return false;
    }

    return true;
}

void CServer::RequestServer(int type, int action, CClient * client, CChannel * chan){

    QString t = QString::number(type);
    QString a = QString::number(action);
    CPacket request(t,a);

    //Récupération du type
    switch (type) {
    case 0:{ //SERV
            switch (action)
            {
            case 0:
            {
                //New User is now online
                request.Serialize_newClient(client);
                sendToServer(request.GetByteArray());

                break;
            }
            case 1:
            {
                //User is now offline
                request.Serialize_newClient(client);
                sendToServer(request.GetByteArray());
                break;
            }
            case 2:
            {
                //PSEUDO UPDATE
                request.Serialize_newClient(client);
                sendToServer(request.GetByteArray());
                break;
            }
            case 3:
            {
                //Bio update
                request.Serialize_newClient(client);
                sendToServer(request.GetByteArray());
                break;
            }
            case 4:
            {
                //BAN USER
                request.Serialize_newClient(client);
                sendToServer(request.GetByteArray());

                break;
            }
            case 5:{
                //BAN IP
                //Rajouter système de gestion du temps
                break;
                }
            case 6: {
                //Kick user
                request.Serialize_newClient(client);
                sendToServer(request.GetByteArray());
                break;
                }

        break;
        }
    case 1: //CHAN
        switch (action)
        {
        case 0: {
            //CONNECT CHAN
            request.Serialize_ID(chan->get_id(),m_self->get_uuid());
            sendToServer(request.GetByteArray());
            break;
        }
        case 1: {
            //QUIT CHAN

            break;
        }
        case 5: {
            //Create chan voc


            break;
        }
        case 6: {
            //Delete chan voc

            break;
        }
        case 7: {
            //Rename chan voc

            break;
        }
        case 8: {
            //Modif max user (voc)

            break;
        }
        case 9: {
            //kick user voc

            break;
        }
        case 10: {
            //Mute user voc (server side)

            break;
        }
        case 11:
            //Create chan text --------> Qxmpp
            break;
        case 12:
            //Delete cahn text
            break;
        case 13:
            //Rename chan text
            break;
        default:
            qDebug() << "Error invalid action" << Qt::endl;
        }
        break;
    case 2: //USER
        switch (action)
        {
        case 0:
            //Mute (user side) ?????
            break;
        case 1:
            //Add friend --> later
            break;
        case 2:
            //Del friend
            break;
        case 3:
            //Send msg to friend
            break;
        case 4:
            //Modif pseudo (update bdd)
            break;
        case 5:
            //Change right
            break;
        default:
            qDebug() << "Error invalid action" << Qt::endl;
        }
        break;
    default:
        qDebug() << "That action isn't listed : " << Qt::endl;
    }
    return;
    }

}

bool CServer::sendMessage(QString msg)
{
    CMessage message(m_self->get_uuid().toString(QUuid::WithoutBraces),"1",msg,false);
    CPacket sendMessage;
    if(message.get_isPrivate() ==  true)
    {
        sendMessage.SetType("2");
        sendMessage.SetAction("6");

        sendMessage.Serialize();
    }
    else
    {
        sendMessage.SetType("1");
        sendMessage.SetAction("2");

        sendMessage.Serialize();
    }

    sendMessage.Serialize_Message(message);
    qint64 messageSize = sendMessage.GetByteArray().size();
    qint64 sendedSize = m_socket->write(sendMessage.GetByteArray());
    if(sendedSize == -1)
    {
        qDebug() << "Error in Login, can't write to socket" << Qt::endl;
        return false;
    }
    else
    {
        qDebug() << "sendedSize :" << sendedSize << Qt::endl << "messageSize :" << messageSize << Qt::endl;
    }
    return true;
}



QByteArray CServer::Serialize(){

    QJsonObject obj;
    QJsonArray cArray, sArray;

    foreach(CChannel * c, get_channelList()){
        cArray.append(c->serializeToObj());
    }
    //foreach(CClient * c, get_clientList()){
       // sArray.append(c->serializeToObj());
    //}
    obj["channels"] = cArray;
    //obj["clients"] = sArray;



    QJsonDocument jsonDoc(obj);
    qDebug() << jsonDoc;

    return jsonDoc.toJson();

}

void CServer::Deserialize(QByteArray in){

    QJsonDocument jsonDoc = QJsonDocument::fromJson(in);
    QJsonObject obj = jsonDoc.object();

    QJsonArray sArray = obj["clients"].toArray();
    QJsonArray cArray = obj["channels"].toArray();

    deserializeChannel(cArray);
    deserializeClients(sArray);

}

QByteArray CServer::SerializeChannels(){
    QJsonArray jsonArray;
    foreach(CChannel * c, get_channelList()){
        jsonArray.append(c->serializeToObj());
    }
    QJsonDocument jsonDoc(jsonArray);
    qDebug() << jsonDoc;

    return jsonDoc.toJson();

}

QByteArray CServer::SerializeClients(){
    QJsonArray jsonArray;
    foreach(CClient * c, get_clientList()){
        jsonArray.append(c->serializeToObj());
    }
    QJsonDocument jsonDoc(jsonArray);
    qDebug() << jsonDoc;

    return jsonDoc.toJson();
}

void CServer::DeserializeChannels(QByteArray in){

        //Deserialize byte array into a json document
        QJsonDocument jsonDoc = QJsonDocument::fromJson(in);

        if(jsonDoc.isNull()){
            qDebug() <<"JSON doc is invalid (null)" << Qt::endl;
        }

        QJsonArray jsonArray = jsonDoc.array();

        //Get each element of the array
        foreach( const QJsonValue & value, jsonArray){

            //Convert it to an json object then to a channel
            QJsonObject obj = value.toObject();
            CChannel * newChannel = deserializeToChannel(obj);
            //qDebug() << "Channel : " << newChannel->get_id()<< newChannel->get_name()<< Qt::endl;

            //check if the channel already exist or not
            bool exist = false;
            //if the channel exist, we reload it with new value
            foreach(CChannel * c, get_channelList()){
                if(c->get_id() == newChannel->get_id()){
                    exist = true;
                    c->set_all(newChannel);
                }
            }
            //if the channel doesnt exist, we add it to the list of channel
            if(get_channelList().isEmpty() || exist == false){
                qDebug() << "That channel doesnt exist, gonna create it " << Qt::endl;
                addChannel(newChannel);
            }
        }

        //Print content of the actual list of channel - check
        /*
        foreach(CChannel * c, get_channelList()){
         qDebug() << "Channel :\nName: " << c->get_name()<< Qt::endl;
         qDebug() << "ID: " << c->get_id()<< Qt::endl;
         qDebug() << "MaxUsers: " << c->get_maxUsers()<< Qt::endl;
         qDebug() << "NbClients: " << c->get_nbClients()<< Qt::endl;
        }*/

}

CChannel * CServer::deserializeToChannel(QJsonObject json_obj){
    CChannel * channel = new CChannel();

    channel->deserialize(json_obj);

    return channel;
}

CClient * CServer::deserializeToClient(QJsonObject json_obj){
    CClient * client = new CClient();
    client->deserialize(json_obj);
    return client;
}

void CServer::deserializeChannel(QJsonArray & json_array){

    foreach( const QJsonValue & value, json_array){

        //Convert it to an json object then to a channel
        QJsonObject obj = value.toObject();
        CChannel * newChannel = deserializeToChannel(obj);


        //check if the channel already exist or not
        bool exist = false;
        foreach(CChannel * c, get_channelList()){
            if(c->get_id() == newChannel->get_id())
                 exist = true;
        }

        if(exist == false){
            addChannel(newChannel);
        }
    }
}

void CServer::deserializeClients(QJsonArray & json_array){

    foreach( const QJsonValue & value, json_array){
        //Convert it to an json object then to a channel
        QJsonObject obj = value.toObject();
        CClient * newClient = deserializeToClient(obj);

        //check if the channel already exist or not
        bool exist = false;
        foreach(CClient * c, get_clientList()){
            if(c->get_uuid() == newClient->get_uuid())
                 exist = true;
        }

        if(exist == false)
            addClient(newClient);

    }
}
