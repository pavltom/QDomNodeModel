//  Copyright (c) 2011 Stanislaw Adaszewski, portions (c) 2019 Tomas Pavlicek,
//  (c) 2021 Chris Hennes
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//      * Redistributions of source code must retain the above copyright
//        notice, this list of conditions and the following disclaimer.
//      * Redistributions in binary form must reproduce the above copyright
//        notice, this list of conditions and the following disclaimer in the
//        documentation and/or other materials provided with the distribution.
//      * Neither the name of Stanislaw Adaszewski nor the
//        names of other contributors may be used to endorse or promote products
//        derived from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//  DISCLAIMED. IN NO EVENT SHALL STANISLAW ADASZEWSKI BE LIABLE FOR ANY
//  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#include "QDomNodeModel.h"

#include <QDomNode>
#include <QDomDocument>
#include <QUrl>
#include <QVector>
#include <QSourceLocation>
#include <QVariant>

class PrivateDomNodeWrapper: public QDomNode
{
public:
    PrivateDomNodeWrapper(const QDomNode &other):
        QDomNode(other)
    {
    }

    PrivateDomNodeWrapper(QDomNodePrivate *otherImpl):
        QDomNode(otherImpl)
    {
    }

    QDomNodePrivate *getImpl()
    {
        return impl;
    }
};

QDomNodeModel::QDomNodeModel(QXmlNamePool pool, QDomDocument doc, bool parsedReadOnly):
    m_Pool(pool), m_Doc(doc), m_ReadOnly(parsedReadOnly)
{
}

QUrl QDomNodeModel::baseUri(const QXmlNodeModelIndex &) const
{
    // TODO: Not implemented.
    return QUrl();
}

#include <QDebug>

QXmlNodeModelIndex::DocumentOrder QDomNodeModel::compareOrder(
    const QXmlNodeModelIndex &ni1,
    const QXmlNodeModelIndex &ni2) const
{
    QDomNode n1 = toDomNode(ni1);
    QDomNode n2 = toDomNode(ni2);

    if (n1 == n2) {
        return QXmlNodeModelIndex::Is;
    }

    if (m_ReadOnly) {
        int i1 = n1.lineNumber();
        int i2 = n2.lineNumber();

        if (i1 < i2) {
            return QXmlNodeModelIndex::Precedes;
        }

        if (i1 > i2) {
            return QXmlNodeModelIndex::Follows;
        }

        i1 = n1.columnNumber();
        i2 = n2.columnNumber();

        if (i1 < i2) {
            return QXmlNodeModelIndex::Precedes;
        }

        if (i1 > i2) {
            return QXmlNodeModelIndex::Follows;
        }

        return QXmlNodeModelIndex::Is;
    }

    QVector<QDomNode> p1(path(n1));
    QVector<QDomNode> p2(path(n2));

    if (p1.at(0) != p2.at(0)) {
        return QXmlNodeModelIndex::Is; // When root is not common, return obvious nonsense
    }

    int s = p1.size() < p2.size() ? p1.size() : p2.size();
    for (int i = 1; i < s; ++i) {
        if (p1.at(i) != p2.at(i)) {
            QDomNode c = p1.at(i - 1).firstChild();
            while (!c.isNull()) {
                if (c == p1.at(i)) {
                    return QXmlNodeModelIndex::Precedes;
                }
                if (c == p2.at(i)) {
                    return QXmlNodeModelIndex::Follows;
                }

                c = c.nextSibling();
            }

            return QXmlNodeModelIndex::Is; // Should be impossible!
        }
    }

    return QXmlNodeModelIndex::Is; // Should be impossible!
}

QUrl QDomNodeModel::documentUri(const QXmlNodeModelIndex &) const
{
    // TODO: Not implemented.
    return QUrl();
}

QXmlNodeModelIndex QDomNodeModel::elementById(const QXmlName &id) const
{
    return fromDomNode(m_Doc.elementById(id.toClarkName(m_Pool)));
}

QXmlNodeModelIndex::NodeKind QDomNodeModel::kind(const QXmlNodeModelIndex &ni) const
{
    QDomNode n = toDomNode(ni);
    if (n.isAttr()) {
        return QXmlNodeModelIndex::Attribute;
    }
    else if (n.isText()) {
        return QXmlNodeModelIndex::Text;
    }
    else if (n.isComment()) {
        return QXmlNodeModelIndex::Comment;
    }
    else if (n.isDocument()) {
        return QXmlNodeModelIndex::Document;
    }
    else if (n.isElement()) {
        return QXmlNodeModelIndex::Element;
    }
    else if (n.isProcessingInstruction()) {
        return QXmlNodeModelIndex::ProcessingInstruction;
    }

    return static_cast<QXmlNodeModelIndex::NodeKind>(0);
}

QXmlName QDomNodeModel::name(const QXmlNodeModelIndex &ni) const
{
    QDomNode n = toDomNode(ni);

    if (n.isAttr() || n.isElement()) {
        if (!n.namespaceURI().isEmpty()) {
            return QXmlName(m_Pool, n.localName(), n.namespaceURI(), n.prefix());
        }

        QString p = n.prefix();
        QString t = n.nodeName();

        if (p.isEmpty()) {
            int c = t.indexOf(QLatin1Char(':'));
            if (c < 0) {
                p = QString::fromUtf8("");
            }
            else {
                p = t.left(c);
                t = t.mid(c + 1);
            }
        }

        // STANDARD: Namespaces in XML 1.0 - Namespace Defaulting
        // https://www.w3.org/TR/xml-names/#defaulting
        // "The namespace name for an unprefixed attribute name always has no value."
        if (n.isAttr() && p.isEmpty()) {
            return QXmlName(m_Pool, t, QString::fromUtf8(""), p);
        }

        QVector<QXmlName> ns(namespaceBindings(ni));
        for (int i = 0; i < ns.size(); ++i) {
            if (ns.at(i).prefix(m_Pool) == p) {
                return QXmlName(m_Pool, t, ns.at(i).namespaceUri(m_Pool), p);
            }
         }

        return QXmlName(m_Pool, n.nodeName(), QString(), p);
    }

    return QXmlName(m_Pool, n.nodeName(), QString(), QString());
}

QVector<QXmlName> QDomNodeModel::namespaceBindings(const QXmlNodeModelIndex &ni) const
{
    QDomNode n = toDomNode(ni);
    bool xmlNamespaceDefined = false;

    QVector<QXmlName> result;
    while (!n.isNull())
    {
        QDomNamedNodeMap attrs = n.attributes();
        for (int i = 0; i < attrs.size(); ++i)
        {
            QString a = attrs.item(i).nodeName();

            QString p;
            if (a == QString::fromUtf8("xmlns")) {
                p = QString::fromUtf8("");
            }
            else if (a.startsWith(QString::fromUtf8("xmlns:"))) {
                p = a.mid(6);
            }

            if (!p.isNull()) {
                int j;
                for (j = 0; j < result.size(); ++j) {
                    if (result.at(j).prefix(m_Pool) == p) break;
                }

                if (j >= result.size()) {
                    result.append(QXmlName(m_Pool, QString::fromUtf8("xmlns"), attrs.item(i).nodeValue(), p));

                    if (p == QString::fromUtf8("xml")) {
                        xmlNamespaceDefined = true;
                    }
                }
             }
        }

        n = n.parentNode();
    }

    // STANDARD: Namespaces in XML 1.0 - Declaring Namespaces
    // https://www.w3.org/TR/xml-names/#ns-decl
    // "The prefix xml is by definition bound to the namespace name http://www.w3.org/XML/1998/namespace.
    //  It MAY, but need not, be declared, and MUST NOT be bound to any other namespace name. Other prefixes
    //  MUST NOT be bound to this namespace name, and it MUST NOT be declared as the default namespace."
    if (!xmlNamespaceDefined) {
        result.append(QXmlName(m_Pool, QString::fromUtf8("xmlns"),
                      QString::fromUtf8("http://www.w3.org/XML/1998/namespace"), QString::fromUtf8("xml")));
    }

    return result;
}

QVector<QXmlNodeModelIndex> QDomNodeModel::nodesByIdref(const QXmlName &) const
{
    // TODO: Not implemented.
    return QVector<QXmlNodeModelIndex>();
}

QXmlNodeModelIndex QDomNodeModel::root(const QXmlNodeModelIndex &ni) const
{
    QDomNode n = toDomNode(ni);
    while (!n.parentNode().isNull()) {
        n = n.parentNode();
    }

    return fromDomNode(n);
}

QString QDomNodeModel::stringValue(const QXmlNodeModelIndex &ni) const
{
    QDomNode n = toDomNode(ni);

    if (n.isProcessingInstruction()) {
        return n.toProcessingInstruction().data();
    }
    else if (n.isText()) {
        return n.toText().data();
    }
    else if (n.isComment()) {
        return n.toComment().data();
    }
    else if (n.isElement()) {
        return n.toElement().text();
    }
    else if (n.isDocument()) {
        return n.toDocument().documentElement().text();
    }
    else if (n.isAttr()) {
        return n.toAttr().value();
    }

    return QString();
}

QVariant QDomNodeModel::typedValue(const QXmlNodeModelIndex &ni) const
{
    return QVariant::fromValue(stringValue(ni));
}

QXmlNodeModelIndex QDomNodeModel::fromDomNode(const QDomNode &n) const
{
    if (n.isNull()) {
        return QXmlNodeModelIndex();
    }

    return createIndex(PrivateDomNodeWrapper(n).getImpl(), 0);
}

QDomNode QDomNodeModel::toDomNode(const QXmlNodeModelIndex &ni) const
{
    return PrivateDomNodeWrapper(reinterpret_cast<QDomNodePrivate *>(ni.data()));
}

QVector<QDomNode> QDomNodeModel::path(const QDomNode &n) const
{
    QVector<QDomNode> result;

    QDomNode current = n;
    while (!current.isNull()) {
        result.push_back(current);
        current = current.parentNode();
    }

    std::reverse(result.begin(), result.end());
    return result;
}

int QDomNodeModel::childIndex(const QDomNode &n) const
{
    QDomNodeList children = n.parentNode().childNodes();
    for (int i = 0; i < children.size(); ++i) {
        if (children.at(i) == n) {
            return i;
        }
    }

    return -1;
}

QVector<QXmlNodeModelIndex> QDomNodeModel::attributes(const QXmlNodeModelIndex &element) const
{
    QDomElement n = toDomNode(element).toElement();
    QDomNamedNodeMap attrs = n.attributes();

    QVector<QXmlNodeModelIndex> result;
    for (int i = 0; i < attrs.size(); ++i) {
        result.push_back(fromDomNode(attrs.item(i)));
    }

    return result;
}

QXmlNodeModelIndex QDomNodeModel::nextFromSimpleAxis(SimpleAxis axis, const QXmlNodeModelIndex &origin) const
{
    QDomNode n = toDomNode(origin);
    switch(axis) {
        case Parent:
            return fromDomNode(n.parentNode());
        case FirstChild:
            return fromDomNode(n.firstChild());
        case PreviousSibling:
            return fromDomNode(n.previousSibling());
        case NextSibling:
            return fromDomNode(n.nextSibling());
        default:
            return QXmlNodeModelIndex();
    }
}
