#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QtCore>
#include <QFileDialog>
#include <QIODevice>
Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    m_portNameList = getPortNameList();
    ui->comboBox_portNameA->addItems(m_portNameList);
    ui->comboBox_portNameB->addItems(m_portNameList);

    //设置默认波特率
    ui->comboBox_baudRateA->setCurrentIndex(6);//115200    
    ui->comboBox_baudRateB->setCurrentIndex(6);//115200
    if(ui->comboBox_portNameB->count()>2)
    {
        ui->comboBox_portNameA->setCurrentIndex(0);
        ui->comboBox_portNameB->setCurrentIndex(1);
    }    


   m_fileA = nullptr;
   m_fileB = nullptr;
   m_DatastreamA = nullptr;
   m_DatastreamB = nullptr;

    m_showTimer = new QTimer(this);

    m_showTimer->start(10); /*开启定时器，并且每10ms后询问一次。定时的时间一到，马上产生timeout（）信号，继续执行自定义槽函数*/

}

Widget::~Widget()
{
    delete ui;
}
QStringList Widget::getPortNameList()
{
    QStringList serialPortName;
    //查找可用的串口
    foreach(const QSerialPortInfo &Info,QSerialPortInfo::availablePorts())//读取串口信息
    {
        serialPortName << Info.portName();
        qDebug()<<"portname: "<<Info.portName();//调试时可以看的串口信息
    }
    return serialPortName;
}

void Widget::on_pushButton_comopenA_clicked()
{
    //对串口进行一些初始化
    if(ui->pushButton_comopenA->text()=="打开")
    {        
         PortA = new SerialPort(COM0,ui->comboBox_portNameA->currentText(), ui->comboBox_baudRateA->currentText().toInt());

        //接收从子线程传输数据的信号
        connect(PortA,SIGNAL(receive_data(int,char)),this,SLOT(on_showData(int,char)));//,Qt::QueuedConnection
        connect(this,SIGNAL(writePort_sig(int,QByteArray)),PortA,SLOT(write_data(int,QByteArray)));
        connect(this,SIGNAL(closePort_sig(int)),PortA,SLOT(closePort(int)));
        //关闭设置菜单使能
        ui->comboBox_portNameA->setEnabled(false);
        ui->comboBox_baudRateA->setEnabled(false);
        ui->pushButton_comopenA->setText("关闭");
    }
    else
    {
        //关闭线程
        if(PortA!=nullptr)
        {
            emit closePort_sig(COM0);
        }
        //恢复设置菜单使能
        ui->comboBox_portNameA->setEnabled(true);
        ui->comboBox_baudRateA->setEnabled(true);
        ui->pushButton_comopenA->setText("打开");
    }
}
void Widget::on_pushButton_comopenB_clicked()
{
    //对串口进行一些初始化
    if(ui->pushButton_comopenB->text()=="打开")
    {
        PortB = new SerialPort(COM1,ui->comboBox_portNameB->currentText(), ui->comboBox_baudRateB->currentText().toInt());

       //接收从子线程传输数据的信号
        connect(PortB,SIGNAL(receive_data(int,char)),this,SLOT(on_showData(int,char)));//,Qt::QueuedConnection
        connect(this,SIGNAL(writePort_sig(int,QByteArray)),PortB,SLOT(write_data(int,QByteArray)));
        connect(this,SIGNAL(closePort_sig(int)),PortB,SLOT(closePort(int)));


       //关闭设置菜单使能
       ui->comboBox_portNameB->setEnabled(false);
       ui->comboBox_baudRateB->setEnabled(false);
       ui->pushButton_comopenB->setText("关闭");
    }
    else
    {
        //关闭线程
        if(PortB!=nullptr)
        {
             emit closePort_sig(COM1);
        }
        //恢复设置菜单使能
        ui->comboBox_portNameB->setEnabled(true);
        ui->comboBox_baudRateB->setEnabled(true);
        ui->pushButton_comopenB->setText("打开");
    }
}
void Widget::on_pushButton_sendA_clicked()
{
    emit writePort_sig(COM0,ui->lineEdit_writeA->text().toUtf8());
    if(ui->checkBoxA->isChecked())
    {
        emit writePort_sig(COM0,"\n");
    }
}

void Widget::on_pushButton_sendB_clicked()
{
    emit writePort_sig(COM1,ui->lineEdit_writeB->text().toUtf8());
    if(ui->checkBoxB->isChecked())
    {
        emit writePort_sig(COM1,"\n");
    }
}

void Widget::on_pushData(int portId, int newobs)
{
    QTextEdit* pTextEd=nullptr;

    QString msg;

    switch(portId)
    {
    case COM0:
        pTextEd = ui->textEdit_readC;  
        break;
    case COM1:
        pTextEd = ui->textEdit_readD;
        break;
    default:
        break;
    }

    msg.sprintf("%d\n", newobs);
    pTextEd->moveCursor(QTextCursor::End);
    pTextEd->insertPlainText(msg);
}


void Widget::on_showData(int portId, char buff)
{
    QTextEdit* pTextEd=nullptr;

    QString byte = QString(buff);
    switch(portId)
    {
    case COM0:
        pTextEd = ui->textEdit_readA;
        if(m_DatastreamA != nullptr)
        {
            *m_DatastreamA << buff;       // 将数据写入到数据流中，注意是带*
        }
        break;
    case COM1:
        pTextEd = ui->textEdit_readB;
        if(m_DatastreamB != nullptr)
        {
            *m_DatastreamB << buff;       // 将数据写入到数据流中，注意是带*
        }
        break;
    default:
        pTextEd = nullptr;
        break;
    }
    pTextEd->moveCursor(QTextCursor::End);
    pTextEd->insertPlainText(byte);

}

void Widget::on_pushButton_saveA_clicked()
{
    if(ui->pushButton_saveA->text()=="保存")
    {
        QString curPath=QDir::currentPath();//获取系统当前目录

        QDateTime local(QDateTime::currentDateTime());
        QString localTime = local.toString("portA_yyyy-MM-dd_hh:mm:ss");
        QString aFileName= curPath +"/"+ localTime+".dat";
       m_fileA = new QFile(aFileName);
       m_DatastreamA = new QDataStream(m_fileA); //用法
       m_DatastreamA->setVersion(QDataStream::Qt_5_9);

//       https://blog.csdn.net/qq_38832450/article/details/102587454
       m_fileA->open(QIODevice::ReadWrite | QIODevice::Append); //只写 与 添加 打开
       ui->pushButton_saveA->setText("停止");
    }
    else
    {
        if(m_fileA != nullptr)
        {
            m_fileA->close();
            delete m_fileA;
            m_fileA = nullptr;
        }

        if(m_DatastreamA != nullptr)
        {
            delete m_DatastreamA;
            m_DatastreamA = nullptr;
        }

        ui->pushButton_saveA->setText("保存");
    }
}

void Widget::on_pushButton_saveB_clicked()
{
    if(ui->pushButton_saveB->text()=="保存")
    {
        QString curPath=QDir::currentPath();//获取系统当前目录

        QDateTime local(QDateTime::currentDateTime());
        QString localTime = local.toString("portB_yyyy-MM-dd_hh:mm:ss");
        QString aFileName= curPath +"/"+ localTime+".dat";
       m_fileB = new QFile(aFileName);
       m_DatastreamB = new QDataStream(m_fileB); //用法
       m_DatastreamB->setVersion(QDataStream::Qt_5_9);
       m_fileB->open(QIODevice::ReadWrite | QIODevice::Append); //只写 与 添加 打开
       ui->pushButton_saveB->setText("停止");
    }
    else
    {
        if(m_fileB != nullptr)
        {
            m_fileB->close();
            delete m_fileB;
            m_fileB = nullptr;
        }

        if(m_DatastreamB != nullptr)
        {
            delete m_DatastreamB;
            m_DatastreamB = nullptr;
        }

        ui->pushButton_saveB->setText("保存");
    }
}

void Widget::on_pushButton_proc_clicked()
{
    connect(this,SIGNAL(RoverObsReady()),this,SLOT(procStart()));
}
void Widget::procStart()
{


}
void Widget::Rtk()
{

}
