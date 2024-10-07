#ifndef NNFORWARDER_H
#define NNFORWARDER_H

#include <QObject>

class NNForwarder : public QObject
{
    Q_OBJECT
public:
    explicit NNForwarder(QObject *parent = nullptr);

signals:

};

#endif // NNFORWARDER_H
