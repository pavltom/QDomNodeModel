#ifndef _QDOMNODEMODEL_H_
#define _QDOMNODEMODEL_H_

#include <QAbstractXmlNodeModel>
#include <QXmlNamePool>
#include <QDomDocument>

class QDomNodeModel: public QAbstractXmlNodeModel
{
public:
    QDomNodeModel(QXmlNamePool, QDomDocument, bool parsedReadOnly = false);
    virtual QUrl baseUri(const QXmlNodeModelIndex &ni) const;
    virtual QXmlNodeModelIndex::DocumentOrder compareOrder(const QXmlNodeModelIndex &ni1, const QXmlNodeModelIndex &ni2 ) const;
    virtual QUrl documentUri(const QXmlNodeModelIndex &ni) const;
    virtual QXmlNodeModelIndex elementById(const QXmlName &id) const;
    virtual QXmlNodeModelIndex::NodeKind kind(const QXmlNodeModelIndex &ni) const;
    virtual QXmlName name(const QXmlNodeModelIndex &ni) const;
    virtual QVector<QXmlName> namespaceBindings(const QXmlNodeModelIndex &ni) const;
    virtual QVector<QXmlNodeModelIndex> nodesByIdref(const QXmlName &idref) const;
    virtual QXmlNodeModelIndex root(const QXmlNodeModelIndex &ni) const;
    virtual QString stringValue(const QXmlNodeModelIndex &ni) const;
    virtual QVariant typedValue(const QXmlNodeModelIndex &ni) const;

public:
    virtual QXmlNodeModelIndex fromDomNode(const QDomNode &n) const;
    virtual QDomNode toDomNode(const QXmlNodeModelIndex &ni) const;
    virtual QVector<QDomNode> path(const QDomNode &n) const;
    virtual int childIndex(const QDomNode &n) const;

protected:
    virtual QVector<QXmlNodeModelIndex> attributes(const QXmlNodeModelIndex &element) const;
    virtual QXmlNodeModelIndex nextFromSimpleAxis(SimpleAxis axis, const QXmlNodeModelIndex &origin) const;

    mutable QXmlNamePool m_Pool;
    mutable QDomDocument m_Doc;
    bool m_ReadOnly;
};

#endif // _QDOMNODEMODEL_H_
