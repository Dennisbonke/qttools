/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "node.h"

#include "codemarker.h"
#include "config.h"
#include "cppcodeparser.h"
#include "enumnode.h"
#include "functionnode.h"
#include "generator.h"
#include "propertynode.h"
#include "qdocdatabase.h"
#include "qmltypenode.h"
#include "qmlpropertynode.h"
#include "relatedclass.h"
#include "sharedcommentnode.h"
#include "tokenizer.h"
#include "tree.h"
#include "typedefnode.h"

#include <QtCore/quuid.h>
#include <QtCore/qversionnumber.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

int Node::propertyGroupCount_ = 0;
QStringMap Node::operators_;
QMap<QString, Node::NodeType> Node::goals_;

/*!
  \class Node
  \brief The Node class is the base class for all the nodes in QDoc's parse tree.

  Class Node is the base class of all the node subclasses. There is a subclass of Node
  for each type of entity that QDoc can document. The types of entities that QDoc can
  document are listed in the enum type NodeType.

  After ClangCodeParser has parsed all the header files to build its precompiled header,
  it then visits the clang Abstract Syntax Tree (AST). For each node in the AST that it
  determines is in the public API and must be documented, it creates an instance of one
  of the Node subclasses and adds it to the QDoc Tree.

  Each instance of a sublass of Node has a parent pointer to link it into the Tree. The
  parent pointer is obtained by calling \l {parent()}, which returns a pointer to an
  instance of the Node subclass, Aggregate, which is never instantiated directly, but
  as the base class for certain subclasses of Node that can have children. For example,
  ClassNode and QmlTypeNode can have children, so they both inherit Aggregate, while
  PropertyNode and QmlPropertyNode can not have children, so they both inherit Node.

  \sa Aggregate, ClassNode, PropertyNode
 */

/*!
  Initialize the map of search goals. This is called once
  by QDocDatabase::initializeDB(). The map key is a string
  representing a value in the enum Node::NodeType. The map value
  is the enum value.

  There should be an entry in the map for each value in the
  NodeType enum.
 */
void Node::initialize()
{
    goals_.insert("namespace", Node::Namespace);
    goals_.insert("class", Node::Class);
    goals_.insert("struct", Node::Struct);
    goals_.insert("union", Node::Union);
    goals_.insert("header", Node::HeaderFile);
    goals_.insert("headerfile", Node::HeaderFile);
    goals_.insert("page", Node::Page);
    goals_.insert("enum", Node::Enum);
    goals_.insert("example", Node::Example);
    goals_.insert("externalpage", Node::ExternalPage);
    goals_.insert("typedef", Node::Typedef);
    goals_.insert("typealias", Node::TypeAlias);
    goals_.insert("function", Node::Function);
    goals_.insert("proxy", Node::Proxy);
    goals_.insert("property", Node::Property);
    goals_.insert("variable", Node::Variable);
    goals_.insert("group", Node::Group);
    goals_.insert("module", Node::Module);
    goals_.insert("qmltype", Node::QmlType);
    goals_.insert("qmlmodule", Node::QmlModule);
    goals_.insert("qmlproperty", Node::QmlProperty);
    goals_.insert("qmlsignal", Node::Function);
    goals_.insert("qmlsignalhandler", Node::Function);
    goals_.insert("qmlmethod", Node::Function);
    goals_.insert("qmlbasictype", Node::QmlBasicType);
    goals_.insert("sharedcomment", Node::SharedComment);
    goals_.insert("collection", Node::Collection);
}

/*!
  If this Node's type is \a from, change the type to \a to
  and return \c true. Otherwise return false. This function
  is used to change Qml node types to Javascript node types,
  because these nodes are created as Qml nodes before it is
  discovered that the entity represented by the node is not
  Qml but javascript.

  Note that if the function returns true, which means the node
  type was indeed changed, then the node's Genus is also changed
  from QML to JS.

  The function also works in the other direction, but there is
  no use case for that.
 */
bool Node::changeType(NodeType from, NodeType to)
{
    if (nodeType_ == from) {
        nodeType_ = to;
        switch (to) {
        case QmlType:
        case QmlModule:
        case QmlProperty:
        case QmlBasicType:
            setGenus(Node::QML);
            break;
        case JsType:
        case JsModule:
        case JsProperty:
        case JsBasicType:
            setGenus(Node::JS);
            break;
        default:
            setGenus(Node::CPP);
            break;
        }
        return true;
    }
    return false;
}

/*!
  Returns \c true if the node \a n1 is less than node \a n2. The
  comparison is performed by comparing properties of the nodes
  in order of increasing complexity.
 */
bool Node::nodeNameLessThan(const Node *n1, const Node *n2)
{
#define LT_RETURN_IF_NOT_EQUAL(a, b)                                                               \
    if ((a) != (b))                                                                                \
        return (a) < (b);

    if (n1->isPageNode() && n2->isPageNode()) {
        LT_RETURN_IF_NOT_EQUAL(n1->fullName(), n2->fullName());
        LT_RETURN_IF_NOT_EQUAL(n1->fullTitle(), n2->fullTitle());
    }

    if (n1->isFunction() && n2->isFunction()) {
        const FunctionNode *f1 = static_cast<const FunctionNode *>(n1);
        const FunctionNode *f2 = static_cast<const FunctionNode *>(n2);

        LT_RETURN_IF_NOT_EQUAL(f1->isConst(), f2->isConst());
        LT_RETURN_IF_NOT_EQUAL(f1->signature(false, false), f2->signature(false, false));
    }

    LT_RETURN_IF_NOT_EQUAL(n1->location().filePath(), n2->location().filePath());
    LT_RETURN_IF_NOT_EQUAL(n1->nodeType(), n2->nodeType());
    LT_RETURN_IF_NOT_EQUAL(n1->name(), n2->name());
    LT_RETURN_IF_NOT_EQUAL(n1->access(), n2->access());

    return false;
}

/*!
  \enum Node::NodeType

  An unsigned char value that identifies an object as a
  particular subclass of Node.

  \value NoType The node type has not been set yet.
  \value Namespace The Node subclass is NamespaceNode, which represents a C++
         namespace.
  \value Class The Node subclass is ClassNode, which represents a C++ class.
  \value Struct The Node subclass is ClassNode, which represents a C struct.
  \value Union The Node subclass is ClassNode, which represents a C union
         (currently no known uses).
  \value HeaderFile The Node subclass is HeaderNode, which represents a header
         file.
  \value Page The Node subclass is PageNode, which represents a text page from
         a .qdoc file.
  \value Enum The Node subclass is EnumNode, which represents an enum type or
         enum class.
  \value Example The Node subclass is ExampleNode, which represents an example
         subdirectory.
  \value ExternalPage The Node subclass is ExternalPageNode, which is for
         linking to an external page.
  \value Function The Node subclass is FunctionNode, which can represent C++,
         QML, and Javascript functions.
  \value Typedef The Node subclass is TypedefNode, which represents a C++
         typedef.
  \value Property The Node subclass is PropertyNode, which represents a use of
         Q_Property.
  \value Variable The Node subclass is VariableNode, which represents a global
         variable or class data member.
  \value Group The Node subclass is CollectionNode, which represents a group of
         documents.
  \value Module The Node subclass is CollectionNode, which represents a C++
         module.
  \value QmlType The Node subclass is QmlTypeNode, which represents a QML type.
  \value QmlModule The Node subclass is CollectionNode, which represents a QML
         module.
  \value QmlProperty The Node subclass is QmlPropertyNode, which represents a
         property in a QML type.
  \value QmlBasicType The Node subclass is QmlBasicTypeNode, which represents a
         basic type like int, etc.
  \value JsType The Node subclass is QmlTypeNode, which represents a javascript
         type.
  \value JsModule The Node subclass is CollectionNode, which represents a
         javascript module.
  \value JsProperty The Node subclass is QmlPropertyNode, which represents a
         javascript property.
  \value JsBasicType The Node subclass is QmlBasicTypeNode, which represents a
         basic type like int, etc.
  \value SharedComment The Node subclass is SharedCommentNode, which represents
         a collection of nodes that share the same documentation comment.
  \omitvalue Collection
  \value Proxy The Node subclass is ProxyNode, which represents one or more
         entities that are documented in the current module but which actually
         reside in a different module.
  \omitvalue LastType
*/

/*!
  \enum Node::Genus

  An unsigned char value that specifies whether the Node represents a
  C++ element, a QML element, a javascript element, or a text document.
  The Genus values are also passed to search functions to specify the
  Genus of Tree Node that can satisfy the search.

  \value DontCare The Genus is not specified. Used when calling Tree search functions to indicate
                  the search can accept any Genus of Node.
  \value CPP The Node represents a C++ element.
  \value JS The Node represents a javascript element.
  \value QML The Node represents a QML element.
  \value DOC The Node represents a text document.
*/

/*!
  \enum Access

  An unsigned char value that indicates the C++ access level.

  \value Public The element has public access.
  \value Protected The element has protected access.
  \value Private The element has private access.
*/

/*!
  \enum Node::Status

  An unsigned char that specifies the status of the documentation element in
  the documentation set.

  \value Obsolete The element is obsolete and no longer exists in the software.
  \value Deprecated The element has been deprecated but still exists in the software.
  \value Preliminary The element is new; the documentation is preliminary.
  \value Active The element is current.
  \value Internal The element is for internal use only, not to be published.
  \value DontDocument The element is not to be documented.
*/

/*!
  \enum Node::ThreadSafeness

  An unsigned char that specifies the degree of thread-safeness of the element.

  \value UnspecifiedSafeness The thread-safeness is not specified.
  \value NonReentrant The element is not reentrant.
  \value Reentrant The element is reentrant.
  \value ThreadSafe The element is threadsafe.
*/

/*!
  \enum Node::LinkType

  An unsigned char value that probably should be moved out of the Node base class.

  \value StartLink
  \value NextLink
  \value PreviousLink
  \value ContentsLink
 */

/*!
  \enum Node::PageType

  An unsigned char value that indicates what kind of documentation page
  the Node represents. I think it is not very useful anymore.

  \value NoPageType
  \value AttributionPage
  \value ApiPage
  \value ArticlePage
  \value ExamplePage
  \value HowToPage
  \value OverviewPage
  \value TutorialPage
  \value FAQPage
  \omitvalue OnBeyondZebra
*/

/*!
  \enum Node::FlagValue

  A value used in PropertyNode and QmlPropertyNode that can be -1, 0, or +1.
  Properties and QML properties have flags, which can be 0 or 1, false or true,
  or not set. FlagValueDefault is the not set value. In other words, if a flag
  is set to FlagValueDefault, the meaning is the flag has not been set.

  \value FlagValueDefault -1 Not set.
  \value FlagValueFalse 0 False.
  \value FlagValueTrue 1 True.
*/

/*!
  \fn Node::~Node()

  The default destructor is virtual so any subclass of Node can be
  deleted by deleting a pointer to Node.
 */

/*! \fn bool Node::isActive() const
  Returns true if this node's status is \c Active.
 */

/*! \fn bool Node::isAnyType() const
  Always returns true. I'm not sure why this is here.
 */

/*! \fn bool Node::isClass() const
  Returns true if the node type is \c Class.
 */

/*! \fn bool Node::isCppNode() const
  Returns true if this node's Genus value is \c CPP.
 */

/*! \fn bool Node::isDeprecated() const
  Returns true if this node's status is \c Deprecated.
 */

/*! \fn bool Node::isDontDocument() const
  Returns true if this node's status is \c DontDocument.
 */

/*! \fn bool Node::isEnumType() const
  Returns true if the node type is \c Enum.
 */

/*! \fn bool Node::isExample() const
  Returns true if the node type is \c Example.
 */

/*! \fn bool Node::isExternalPage() const
  Returns true if the node type is \c ExternalPage.
 */

/*! \fn bool Node::isFunction(Genus g = DontCare) const
  Returns true if this is a FunctionNode and its Genus is set to \a g.
 */

/*! \fn bool Node::isGroup() const
  Returns true if the node type is \c Group.
 */

/*! \fn bool Node::isHeader() const
  Returns true if the node type is \c HeaderFile.
 */

/*! \fn bool Node::isIndexNode() const
  Returns true if this node was created from something in an index file.
 */

/*! \fn bool Node::isJsBasicType() const
  Returns true if the node type is \c JsBasicType.
 */

/*! \fn bool Node::isJsModule() const
  Returns true if the node type is \c JsModule.
 */

/*! \fn bool Node::isJsNode() const
  Returns true if this node's Genus value is \c JS.
 */

/*! \fn bool Node::isJsProperty() const
  Returns true if the node type is \c JsProperty.
 */

/*! \fn bool Node::isJsType() const
  Returns true if the node type is \c JsType.
 */

/*! \fn bool Node::isModule() const
  Returns true if the node type is \c Module.
 */

/*! \fn bool Node::isNamespace() const
  Returns true if the node type is \c Namespace.
 */

/*! \fn bool Node::isObsolete() const
  Returns true if this node's status is \c Obsolete.
 */

/*! \fn bool Node::isPage() const
  Returns true if the node type is \c Page.
 */

/*! \fn bool Node::isPreliminary() const
  Returns true if this node's status is \c Preliminary.
 */

/*! \fn bool Node::isPrivate() const
  Returns true if this node's access is \c Private.
 */

/*! \fn bool Node::isProperty() const
  Returns true if the node type is \c Property.
 */

/*! \fn bool Node::isProxyNode() const
  Returns true if the node type is \c Proxy.
 */

/*! \fn bool Node::isPublic() const
  Returns true if this node's access is \c Public.
 */

/*! \fn bool Node::isProtected() const
  Returns true if this node's access is \c Protected.
 */

/*! \fn bool Node::isQmlBasicType() const
  Returns true if the node type is \c QmlBasicType.
 */

/*! \fn bool Node::isQmlModule() const
  Returns true if the node type is \c QmlModule.
 */

/*! \fn bool Node::isQmlNode() const
  Returns true if this node's Genus value is \c QML.
 */

/*! \fn bool Node::isQmlProperty() const
  Returns true if the node type is \c QmlProperty.
 */

/*! \fn bool Node::isQmlType() const
  Returns true if the node type is \c QmlType.
 */

/*! \fn bool Node::isRelatedNonmember() const
  Returns true if this is a related nonmember of something.
 */

/*! \fn bool Node::isStruct() const
  Returns true if the node type is \c Struct.
 */

/*! \fn bool Node::isSharedCommentNode() const
  Returns true if the node type is \c SharedComment.
 */

/*! \fn bool Node::isTypeAlias() const
  Returns true if the node type is \c Typedef.
 */

/*! \fn bool Node::isTypedef() const
  Returns true if the node type is \c Typedef.
 */

/*! \fn bool Node::isUnion() const
  Returns true if the node type is \c Union.
 */

/*! \fn bool Node::isVariable() const
  Returns true if the node type is \c Variable.
 */

/*! \fn bool Node::isGenericCollection() const
  Returns true if the node type is \c Collection.
 */

/*! \fn bool Node::isAbstract() const
  Returns true if the ClassNode or QmlTypeNode is marked abstract.
*/

/*! \fn bool Node::isAggregate() const
  Returns true if this node is an aggregate, which means it
  inherits Aggregate and can therefore have children.
*/

/*! \fn bool Node::isFirstClassAggregate() const
  Returns true if this Node is an Aggregate but not a ProxyNode.
*/

/*! \fn bool Node::isAlias() const
  Returns true if this QML property is marked as an alias.
*/

/*! \fn bool Node::isAttached() const
  Returns true if the QML property or QML method node is marked as attached.
*/

/*! \fn bool Node::isClassNode() const
  Returns true if this is an instance of ClassNode.
*/

/*! \fn bool Node::isCollectionNode() const
  Returns true if this is an instance of CollectionNode.
*/

/*! \fn bool Node::isDefault() const
  Returns true if the QML property node is marked as default.
*/

/*! \fn bool Node::isMacro() const
  returns true if either FunctionNode::isMacroWithParams() or
  FunctionNode::isMacroWithoutParams() returns true.
*/

/*! \fn bool Node::isPageNode() const
  Returns true if this node represents something that generates a documentation
  page. In other words, if this Node's subclass inherits PageNode, then this
  function will return \e true.
*/

/*! \fn bool Node::isQtQuickNode() const
  Returns true if this node represents a QML element in the QtQuick module.
*/

/*! \fn bool Node::isReadOnly() const
  Returns true if the QML property node is marked as a read-only property.
*/

/*! \fn bool Node::isRelatableType() const
  Returns true if this node is something you can relate things to with
  the \e relates command. NamespaceNode, ClassNode, HeaderNode, and
  ProxyNode are relatable types.
*/

/*! \fn bool Node::isMarkedReimp() const
  Returns true if the FunctionNode is marked as a reimplemented function.
  That means it is virtual in the base class and it is reimplemented in
  the subclass.
*/

/*! \fn bool Node::isPropertyGroup() const
  Returns true if the node is a SharedCommentNode for documenting
  multiple C++ properties or multiple QML properties.
*/

/*! \fn bool Node::isStatic() const
  Returns true if the FunctionNode represents a static function.
*/

/*! \fn bool Node::isTextPageNode() const
  Returns true if the node is a PageNode but not an Aggregate.
*/

/*!
  Returns this node's name member. Appends "()" to the returned
  name if this node is a function node, but not if it is a macro
  because macro names normally appear without parentheses.
 */
QString Node::plainName() const
{
    if (isFunction() && !isMacro())
        return name_ + QLatin1String("()");
    return name_;
}

/*!
  Constructs and returns the node's fully qualified name by
  recursively ascending the parent links and prepending each
  parent name + "::". Breaks out when reaching a HeaderNode,
  or when the parent pointer is \a relative. Typically, calls
  to this function pass \c nullptr for \a relative.
 */
QString Node::plainFullName(const Node *relative) const
{
    if (name_.isEmpty())
        return QLatin1String("global");
    if (isHeader())
        return plainName();

    QStringList parts;
    const Node *node = this;
    while (node && !node->isHeader()) {
        parts.prepend(node->plainName());
        if (node->parent() == relative || node->parent()->name().isEmpty())
            break;
        node = node->parent();
    }
    return parts.join(QLatin1String("::"));
}

/*!
  Constructs and returns the node's fully qualified signature
  by recursively ascending the parent links and prepending each
  parent name + "::" to the plain signature. The return type is
  not included.
 */
QString Node::plainSignature() const
{
    if (name_.isEmpty())
        return QLatin1String("global");

    QString fullName;
    const Node *node = this;
    while (node) {
        fullName.prepend(node->signature(false, true));
        if (node->parent()->name().isEmpty())
            break;
        fullName.prepend(QLatin1String("::"));
        node = node->parent();
    }
    return fullName;
}

/*!
  Constructs and returns this node's full name. The full name is
  often just the title(). When it is not the title, it is the
  plainFullName().
 */
QString Node::fullName(const Node *relative) const
{
    if ((isTextPageNode() || isGroup()) && !title().isEmpty())
        return title();
    return plainFullName(relative);
}

/*!
  Try to match this node's type with one of the \a types.
  If a match is found, return true. If no match is found,
  return false.
 */
bool Node::match(const QVector<int> &types) const
{
    for (int i = 0; i < types.size(); ++i) {
        if (nodeType() == types.at(i))
            return true;
    }
    return false;
}

/*!
  Sets this Node's Doc to \a doc. If \a replace is false and
  this Node already has a Doc, and if this doc is not marked
  with the \\reimp command, a warning is reported that the
  existing Doc is being overridden, and it reports where the
  previous Doc was found. If \a replace is true, the Doc is
  replaced silently.
 */
void Node::setDoc(const Doc &doc, bool replace)
{
    if (!doc_.isEmpty() && !replace && !doc.isMarkedReimp()) {
        doc.location().warning(QStringLiteral("Overrides a previous doc"));
        doc_.location().warning(QStringLiteral("(The previous doc is here)"));
    }
    doc_ = doc;
}

/*!
  Construct a node with the given \a type and having the
  given \a parent and \a name. The new node is added to the
  parent's child list.
 */
Node::Node(NodeType type, Aggregate *parent, const QString &name)
    : nodeType_(type),
      access_(Access::Public),
      safeness_(UnspecifiedSafeness),
      pageType_(NoPageType),
      status_(Active),
      indexNodeFlag_(false),
      relatedNonmember_(false),
      hadDoc_(false),
      parent_(parent),
      sharedCommentNode_(nullptr),
      name_(name)
{
    if (parent_)
        parent_->addChild(this);
    outSubDir_ = Generator::outputSubdir();
    if (operators_.isEmpty()) {
        operators_.insert("++", "inc");
        operators_.insert("--", "dec");
        operators_.insert("==", "eq");
        operators_.insert("!=", "ne");
        operators_.insert("<<", "lt-lt");
        operators_.insert(">>", "gt-gt");
        operators_.insert("+=", "plus-assign");
        operators_.insert("-=", "minus-assign");
        operators_.insert("*=", "mult-assign");
        operators_.insert("/=", "div-assign");
        operators_.insert("%=", "mod-assign");
        operators_.insert("&=", "bitwise-and-assign");
        operators_.insert("|=", "bitwise-or-assign");
        operators_.insert("^=", "bitwise-xor-assign");
        operators_.insert("<<=", "bitwise-left-shift-assign");
        operators_.insert(">>=", "bitwise-right-shift-assign");
        operators_.insert("||", "logical-or");
        operators_.insert("&&", "logical-and");
        operators_.insert("()", "call");
        operators_.insert("[]", "subscript");
        operators_.insert("->", "pointer");
        operators_.insert("->*", "pointer-star");
        operators_.insert("+", "plus");
        operators_.insert("-", "minus");
        operators_.insert("*", "mult");
        operators_.insert("/", "div");
        operators_.insert("%", "mod");
        operators_.insert("|", "bitwise-or");
        operators_.insert("&", "bitwise-and");
        operators_.insert("^", "bitwise-xor");
        operators_.insert("!", "not");
        operators_.insert("~", "bitwise-not");
        operators_.insert("<=", "lt-eq");
        operators_.insert(">=", "gt-eq");
        operators_.insert("<", "lt");
        operators_.insert(">", "gt");
        operators_.insert("=", "assign");
        operators_.insert(",", "comma");
        operators_.insert("delete[]", "delete-array");
        operators_.insert("delete", "delete");
        operators_.insert("new[]", "new-array");
        operators_.insert("new", "new");
    }
    setPageType(getPageType(type));
    setGenus(getGenus(type));
}

/*!
  Determines the appropriate PageType value for the NodeType
  value \a t and returns that PageType value.
 */
Node::PageType Node::getPageType(Node::NodeType t)
{
    switch (t) {
    case Node::Namespace:
    case Node::Class:
    case Node::Struct:
    case Node::Union:
    case Node::HeaderFile:
    case Node::Enum:
    case Node::Function:
    case Node::Typedef:
    case Node::Property:
    case Node::Variable:
    case Node::QmlType:
    case Node::QmlProperty:
    case Node::QmlBasicType:
    case Node::JsType:
    case Node::JsProperty:
    case Node::JsBasicType:
    case Node::SharedComment:
        return Node::ApiPage;
    case Node::Example:
        return Node::ExamplePage;
    case Node::Page:
    case Node::ExternalPage:
        return Node::NoPageType;
    case Node::Group:
    case Node::Module:
    case Node::QmlModule:
    case Node::JsModule:
    case Node::Collection:
        return Node::OverviewPage;
    case Node::Proxy:
    default:
        return Node::NoPageType;
    }
}

/*!
  Determines the appropriate Genus value for the NodeType
  value \a t and returns that Genus value. Note that this
  function is called in the Node() constructor. It always
  returns Node::CPP when \a t is Node::Function, which
  means the FunctionNode() constructor must determine its
  own Genus value separately, because class FunctionNode
  is a subclass of Node.
 */
Node::Genus Node::getGenus(Node::NodeType t)
{
    switch (t) {
    case Node::Enum:
    case Node::Class:
    case Node::Struct:
    case Node::Union:
    case Node::Module:
    case Node::Typedef:
    case Node::Property:
    case Node::Variable:
    case Node::Function:
    case Node::Namespace:
    case Node::HeaderFile:
        return Node::CPP;
    case Node::QmlType:
    case Node::QmlModule:
    case Node::QmlProperty:
    case Node::QmlBasicType:
        return Node::QML;
    case Node::JsType:
    case Node::JsModule:
    case Node::JsProperty:
    case Node::JsBasicType:
        return Node::JS;
    case Node::Page:
    case Node::Group:
    case Node::Example:
    case Node::ExternalPage:
        return Node::DOC;
    case Node::Collection:
    case Node::SharedComment:
    case Node::Proxy:
    default:
        return Node::DontCare;
    }
}

/*! \fn QString Node::url() const
  Returns the node's URL, which is the url of the documentation page
  created for the node or the url of an external page if the node is
  an ExternalPageNode. The url is used for generating a link to the
  page the node represents.

  \sa Node::setUrl()
 */

/*! \fn void Node::setUrl(const QString &url)
  Sets the node's URL to \a url, which is the url to the page that the
  node represents. This function is only called when an index file is
  read. In other words, a node's url is set when qdoc decides where its
  page will be and what its name will be, which happens when qdoc writes
  the index file for the module being documented.

  \sa QDocIndexFiles
 */

/*!
  Returns this node's page type as a string, for use as an
  attribute value in XML or HTML.
 */
QString Node::pageTypeString() const
{
    return pageTypeString(pageType_);
}

/*!
  Returns the page type \a t as a string, for use as an
  attribute value in XML or HTML.
 */
QString Node::pageTypeString(PageType t)
{
    switch (t) {
    case Node::AttributionPage:
        return QLatin1String("attribution");
    case Node::ApiPage:
        return QLatin1String("api");
    case Node::ArticlePage:
        return QLatin1String("article");
    case Node::ExamplePage:
        return QLatin1String("example");
    case Node::HowToPage:
        return QLatin1String("howto");
    case Node::OverviewPage:
        return QLatin1String("overview");
    case Node::TutorialPage:
        return QLatin1String("tutorial");
    case Node::FAQPage:
        return QLatin1String("faq");
    default:
        return QLatin1String("article");
    }
}

/*!
  Returns this node's type as a string for use as an
  attribute value in XML or HTML.
 */
QString Node::nodeTypeString() const
{
    if (isFunction()) {
        const FunctionNode *fn = static_cast<const FunctionNode *>(this);
        return fn->kindString();
    }
    return nodeTypeString(nodeType());
}

/*!
  Returns the node type \a t as a string for use as an
  attribute value in XML or HTML.
 */
QString Node::nodeTypeString(NodeType t)
{
    switch (t) {
    case Namespace:
        return QLatin1String("namespace");
    case Class:
        return QLatin1String("class");
    case Struct:
        return QLatin1String("struct");
    case Union:
        return QLatin1String("union");
    case HeaderFile:
        return QLatin1String("header");
    case Page:
        return QLatin1String("page");
    case Enum:
        return QLatin1String("enum");
    case Example:
        return QLatin1String("example");
    case ExternalPage:
        return QLatin1String("external page");
    case TypeAlias:
        return QLatin1String("alias");
    case Typedef:
        return QLatin1String("typedef");
    case Function:
        return QLatin1String("function");
    case Property:
        return QLatin1String("property");
    case Proxy:
        return QLatin1String("proxy");
    case Variable:
        return QLatin1String("variable");
    case Group:
        return QLatin1String("group");
    case Module:
        return QLatin1String("module");

    case QmlType:
        return QLatin1String("QML type");
    case QmlBasicType:
        return QLatin1String("QML basic type");
    case QmlModule:
        return QLatin1String("QML module");
    case QmlProperty:
        return QLatin1String("QML property");

    case JsType:
        return QLatin1String("JS type");
    case JsBasicType:
        return QLatin1String("JS basic type");
    case JsModule:
        return QLatin1String("JS module");
    case JsProperty:
        return QLatin1String("JS property");

    case SharedComment:
        return QLatin1String("shared comment");
    case Collection:
        return QLatin1String("collection");
    default:
        break;
    }
    return QString();
}

/*!
  Set the page type according to the string \a t.
 */
void Node::setPageType(const QString &t)
{
    if ((t == "API") || (t == "api"))
        pageType_ = ApiPage;
    else if (t == "howto")
        pageType_ = HowToPage;
    else if (t == "overview")
        pageType_ = OverviewPage;
    else if (t == "tutorial")
        pageType_ = TutorialPage;
    else if (t == "faq")
        pageType_ = FAQPage;
    else if (t == "article")
        pageType_ = ArticlePage;
    else if (t == "example")
        pageType_ = ExamplePage;
}

/*! Converts the boolean value \a b to an enum representation
  of the boolean type, which includes an enum value for the
  \e {default value} of the item, i.e. true, false, or default.
 */
Node::FlagValue Node::toFlagValue(bool b)
{
    return b ? FlagValueTrue : FlagValueFalse;
}

/*!
  Converts the enum \a fv back to a boolean value.
  If \a fv is neither the true enum value nor the
  false enum value, the boolean value returned is
  \a defaultValue.

  Note that runtimeDesignabilityFunction() should be called
  first. If that function returns the name of a function, it
  means the function must be called at runtime to determine
  whether the property is Designable.
 */
bool Node::fromFlagValue(FlagValue fv, bool defaultValue)
{
    switch (fv) {
    case FlagValueTrue:
        return true;
    case FlagValueFalse:
        return false;
    default:
        return defaultValue;
    }
}

/*!
  This function creates a pair that describes a link.
  The pair is composed from \a link and \a desc. The
  \a linkType is the map index the pair is filed under.
 */
void Node::setLink(LinkType linkType, const QString &link, const QString &desc)
{
    QPair<QString, QString> linkPair;
    linkPair.first = link;
    linkPair.second = desc;
    linkMap_[linkType] = linkPair;
}

/*!
    Sets the information about the project and version a node was introduced
    in, unless the version is lower than the 'ignoresince.<project>'
    configuration variable.
 */
void Node::setSince(const QString &since)
{
    QStringList parts = since.split(QLatin1Char(' '));
    QString project;
    if (parts.size() > 1)
        project = Config::dot + parts.first();

    QVersionNumber cutoff =
            QVersionNumber::fromString(Config::instance().getString(CONFIG_IGNORESINCE + project))
                    .normalized();

    if (!cutoff.isNull() && QVersionNumber::fromString(parts.last()).normalized() < cutoff)
        return;

    since_ = parts.join(QLatin1Char(' '));
}

/*!
  Returns a string representing the access specifier.
 */
QString Node::accessString() const
{
    switch (access_) {
    case Access::Protected:
        return QLatin1String("protected");
    case Access::Private:
        return QLatin1String("private");
    case Access::Public:
    default:
        break;
    }
    return QLatin1String("public");
}

/*!
  Extract a class name from the type \a string and return it.
 */
QString Node::extractClassName(const QString &string) const
{
    QString result;
    for (int i = 0; i <= string.size(); ++i) {
        QChar ch;
        if (i != string.size())
            ch = string.at(i);

        QChar lower = ch.toLower();
        if ((lower >= QLatin1Char('a') && lower <= QLatin1Char('z')) || ch.digitValue() >= 0
            || ch == QLatin1Char('_') || ch == QLatin1Char(':')) {
            result += ch;
        } else if (!result.isEmpty()) {
            if (result != QLatin1String("const"))
                return result;
            result.clear();
        }
    }
    return result;
}

/*!
  Returns the thread safeness value for whatever this node
  represents. But if this node has a parent and the thread
  safeness value of the parent is the same as the thread
  safeness value of this node, what is returned is the
  value \c{UnspecifiedSafeness}. Why?
 */
Node::ThreadSafeness Node::threadSafeness() const
{
    if (parent_ && safeness_ == parent_->inheritedThreadSafeness())
        return UnspecifiedSafeness;
    return safeness_;
}

/*!
  If this node has a parent, the parent's thread safeness
  value is returned. Otherwise, this node's thread safeness
  value is returned. Why?
 */
Node::ThreadSafeness Node::inheritedThreadSafeness() const
{
    if (parent_ && safeness_ == UnspecifiedSafeness)
        return parent_->inheritedThreadSafeness();
    return safeness_;
}

/*!
  If this node is a QML or JS type node, return a pointer to
  it. If it is a child of a QML or JS type node, return the
  pointer to its parent QMLor JS type node. Otherwise return
  0;
 */
QmlTypeNode *Node::qmlTypeNode()
{
    if (isQmlNode() || isJsNode()) {
        Node *n = this;
        while (n && !(n->isQmlType() || n->isJsType()))
            n = n->parent();
        if (n && (n->isQmlType() || n->isJsType()))
            return static_cast<QmlTypeNode *>(n);
    }
    return nullptr;
}

/*!
  If this node is a QML node, find its QML class node,
  and return a pointer to the C++ class node from the
  QML class node. That pointer will be null if the QML
  class node is a component. It will be non-null if
  the QML class node is a QML element.
 */
ClassNode *Node::declarativeCppNode()
{
    QmlTypeNode *qcn = qmlTypeNode();
    if (qcn)
        return qcn->classNode();
    return nullptr;
}

/*!
  Returns \c true if the node's status is \c Internal, or if
  its parent is a class with \c Internal status.
 */
bool Node::isInternal() const
{
    if (status() == Internal)
        return true;
    if (parent() && parent()->status() == Internal)
        return true;
    return false;
}

/*! \fn void Node::markInternal()
  Sets the node's access to Private and its status to Internal.
 */

/*!
  Returns a pointer to the root of the Tree this node is in.
 */
Aggregate *Node::root() const
{
    if (parent() == nullptr)
        return (this->isAggregate() ? static_cast<Aggregate *>(const_cast<Node *>(this)) : nullptr);
    Aggregate *t = parent();
    while (t->parent() != nullptr)
        t = t->parent();
    return t;
}

/*!
  Returns a pointer to the Tree this node is in.
 */
Tree *Node::tree() const
{
    return root()->tree();
}

/*!
  Sets the node's declaration location, its definition
  location, or both, depending on the suffix of the file
  name from the file path in location \a t.
 */
void Node::setLocation(const Location &t)
{
    QString suffix = t.fileSuffix();
    if (suffix == "h")
        declLocation_ = t;
    else if (suffix == "cpp")
        defLocation_ = t;
    else {
        declLocation_ = t;
        defLocation_ = t;
    }
}

/*!
  Returns true if this node is sharing a comment and the
  shared comment is not empty.
 */
bool Node::hasSharedDoc() const
{
    return (sharedCommentNode_ && sharedCommentNode_->hasDoc());
}

/*!
  Returns the CPP node's qualified name by prepending the
  namespaces name + "::" if there isw a namespace.
 */
QString Node::qualifyCppName()
{
    if (parent_ && parent_->isNamespace() && !parent_->name().isEmpty())
        return parent_->name() + "::" + name_;
    return name_;
}

/*!
  Return the name of this node qualified with the parent name
  and "::" if there is a parent name.
 */
QString Node::qualifyWithParentName()
{
    if (parent_ && !parent_->name().isEmpty())
        return parent_->name() + "::" + name_;
    return name_;
}

/*!
  Returns the QML node's qualified name by stripping off the
  "QML:" if present and prepending the logical module name.
 */
QString Node::qualifyQmlName()
{
    QString qualifiedName = name_;
    if (name_.startsWith(QLatin1String("QML:")))
        qualifiedName = name_.mid(4);
    qualifiedName = logicalModuleName() + "::" + name_;
    return qualifiedName;
}

/*!
  Returns the QML node's name after stripping off the
  "QML:" if present.
 */
QString Node::unqualifyQmlName()
{
    QString qmlTypeName = name_.toLower();
    if (qmlTypeName.startsWith(QLatin1String("qml:")))
        qmlTypeName = qmlTypeName.mid(4);
    return qmlTypeName;
}

/*!
  Returns \c true if the node is a class node or a QML type node
  that is marked as being a wrapper class or wrapper QML type,
  or if it is a member of a wrapper class or type.
 */
bool Node::isWrapper() const
{
    return (parent_ ? parent_->isWrapper() : false);
}

/*!
  Construct the full document name for this node and return it.
 */
QString Node::fullDocumentName() const
{
    QStringList pieces;
    const Node *n = this;

    do {
        if (!n->name().isEmpty())
            pieces.insert(0, n->name());

        if ((n->isQmlType() || n->isJsType()) && !n->logicalModuleName().isEmpty()) {
            pieces.insert(0, n->logicalModuleName());
            break;
        }

        if (n->isTextPageNode())
            break;

        // Examine the parent if the node is a member
        if (!n->parent() || n->isRelatedNonmember())
            break;

        n = n->parent();
    } while (true);

    // Create a name based on the type of the ancestor node.
    QString concatenator = "::";
    if (n->isQmlType() || n->isJsType())
        concatenator = QLatin1Char('.');

    if (n->isTextPageNode())
        concatenator = QLatin1Char('#');

    return pieces.join(concatenator);
}

/*!
  Returns the \a str as an NCName, which means the name can
  be used as the value of an \e id attribute. Search for NCName
  on the internet for details of what can be an NCName.
 */
QString Node::cleanId(const QString &str)
{
    QString clean;
    QString name = str.simplified();

    if (name.isEmpty())
        return clean;

    name = name.replace("::", "-");
    name = name.replace(QLatin1Char(' '), QLatin1Char('-'));
    name = name.replace("()", "-call");

    clean.reserve(name.size() + 20);
    if (!str.startsWith("id-"))
        clean = "id-";
    const QChar c = name[0];
    const uint u = c.unicode();

    if ((u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z') || (u >= '0' && u <= '9')) {
        clean += c;
    } else if (u == '~') {
        clean += "dtor.";
    } else if (u == '_') {
        clean += "underscore.";
    } else {
        clean += QLatin1Char('a');
    }

    for (int i = 1; i < name.length(); i++) {
        const QChar c = name[i];
        const uint u = c.unicode();
        if ((u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z') || (u >= '0' && u <= '9') || u == '-'
            || u == '_' || u == '.') {
            clean += c;
        } else if (c.isSpace() || u == ':') {
            clean += QLatin1Char('-');
        } else if (u == '!') {
            clean += "-not";
        } else if (u == '&') {
            clean += "-and";
        } else if (u == '<') {
            clean += "-lt";
        } else if (u == '=') {
            clean += "-eq";
        } else if (u == '>') {
            clean += "-gt";
        } else if (u == '#') {
            clean += "-hash";
        } else if (u == '(') {
            clean += QLatin1Char('-');
        } else if (u == ')') {
            clean += QLatin1Char('-');
        } else {
            clean += QLatin1Char('-');
            clean += QString::number((int)u, 16);
        }
    }
    return clean;
}

/*!
  Find the module (Qt Core, Qt GUI, etc.) to which the class belongs.
  We do this by obtaining the full path to the header file's location
  and examine everything between "src/" and the filename. This is
  semi-dirty because we are assuming a particular directory structure.

  This function is only really useful if the class's module has not
  been defined in the header file with a QT_MODULE macro or with an
  \inmodule command in the documentation.
 */
QString Node::physicalModuleName() const
{
    if (!physicalModuleName_.isEmpty())
        return physicalModuleName_;

    QString path = location().filePath();
    QString pattern = QString("src") + QDir::separator();
    int start = path.lastIndexOf(pattern);

    if (start == -1)
        return QString();

    QString moduleDir = path.mid(start + pattern.size());
    int finish = moduleDir.indexOf(QDir::separator());

    if (finish == -1)
        return QString();

    QString physicalModuleName = moduleDir.left(finish);

    if (physicalModuleName == QLatin1String("corelib"))
        return QLatin1String("QtCore");
    else if (physicalModuleName == QLatin1String("uitools"))
        return QLatin1String("QtUiTools");
    else if (physicalModuleName == QLatin1String("gui"))
        return QLatin1String("QtGui");
    else if (physicalModuleName == QLatin1String("network"))
        return QLatin1String("QtNetwork");
    else if (physicalModuleName == QLatin1String("opengl"))
        return QLatin1String("QtOpenGL");
    else if (physicalModuleName == QLatin1String("svg"))
        return QLatin1String("QtSvg");
    else if (physicalModuleName == QLatin1String("sql"))
        return QLatin1String("QtSql");
    else if (physicalModuleName == QLatin1String("qtestlib"))
        return QLatin1String("QtTest");
    else if (moduleDir.contains("webkit"))
        return QLatin1String("QtWebKit");
    else if (physicalModuleName == QLatin1String("xml"))
        return QLatin1String("QtXml");
    else
        return QString();
}

/*! \fn Node *Node::clone(Aggregate *parent)

  When reimplemented in a subclass, this function creates a
  clone of this node on the heap and makes the clone a child
  of \a parent. A pointer to the clone is returned.

  Here in the base class, this function does nothing and returns
  nullptr.
 */

/*! \fn NodeType Node::nodeType() const
  Returns this node's type.

  \sa NodeType
*/

/*! \fn Genus Node::genus() const
  Returns this node's Genus.

  \sa Genus
*/

/*! void Node::setGenus(Genus t)
  Sets this node's Genus to \a t.
*/

/*! \fn PageType Node::pageType() const
  Returns this node's page type.

  \sa PageType
*/

/*! \fn void Node::setPageType(PageType t)
  Sets this node's page type to \a t.

  \sa PageType
*/

/*! \fn  QString Node::signature(bool values, bool noReturnType, bool templateParams) const

  If this node is a FunctionNode, this function returns the function's
  signature, including default values if \a values is \c true,
  function's return type if \a noReturnType is \c false, and
  prefixed with 'template <parameter_list>' for function templates
  if templateParams is \true.

  If this node is not a FunctionNode, this function returns plainName().
*/

/*! \fn const QString &Node::fileNameBase() const
  Returns the node's file name base string, which is built once, when
  Generator::fileBase() is called and stored in the Node.
*/

/*! \fn bool Node::hasFileNameBase() const
  Returns true if the node's file name base has been set.

  \sa Node::fileNameBase()
*/

/*! \fn void Node::setFileNameBase(const QString &t)
  Sets the node's file name base to \a t. Only called by
  Generator::fileBase().
*/

/*! \fn void Node::setAccess(Access t)
  Sets the node's access type to \a t.

  \sa Access
*/

/*! \fn void Node::setStatus(Status t)
  Sets the node's status to \a t, except that once
  the node's status has been set to \c Obsolete, it
  can't be reset to \c Deprecated.

  \sa Status
*/

/*! \fn void Node::setThreadSafeness(ThreadSafeness t)
  Sets the node's thread safeness to \a t.

  \sa ThreadSafeness
*/

/*! \fn void Node::setPhysicalModuleName(const QString &name)
  Sets the node's physical module \a name.
*/

/*! \fn void Node::setReconstitutedBrief(const QString &t)
  When reading an index file, this function is called with the
  reconstituted brief clause \a t to set the node's brief clause.
  I think this is needed for linking to something in the brief clause.
*/

/*! \fn void Node::setParent(Aggregate *n)
  Sets the node's parent pointer to \a n. Such a thing
  is not lightly done. All the calls to this function
  are in other member functions of Node subclasses. See
  the code in node.cpp to understand when this function
  can be called safely and why it is called.
*/

/*! \fn void Node::setIndexNodeFlag(bool isIndexNode = true)
  Sets a flag in this Node that indicates the node was created
  for something in an index file. This is important to know
  because an index node is not to be documented in the current
  module. When the index flag is set, it means the Node
  represents something in another module, and it will be
  documented in that module's documentation.
*/

/*! \fn void Node::setRelatedNonmember(bool b)
  Sets a flag in the node indicating whether this node is a related nonmember
  of something. This function is called when the \c relates command is seen.
 */

/*! \fn void Node::setOutputFileName(const QString &f)
  In a PageNode, this function sets the node's output file name to \a f.
  In a non-PageNode, this function does nothing.
 */

/*! \fn void Node::addMember(Node *node)
  In a CollectionNode, this function adds \a node to the collection
  node's members list. It does nothing if this node is not a CollectionNode.
 */

/*! \fn bool Node::hasMembers() const
  Returns \c true if this is a CollectionNode and its members list
  is not empty. Otherwise it returns \c false.
 */

/*! \fn bool Node::hasNamespaces() const
  Returns \c true if this is a CollectionNode and its members list
  contains namespace nodes. Otherwise it returns \c false.
 */

/*! \fn bool Node::hasClasses() const
  Returns \c true if this is a CollectionNode and its members list
  contains class nodes. Otherwise it returns \c false.
 */

/*! \fn void Node::setAbstract(bool b)
  If this node is a ClassNode or a QmlTypeNode, the node's abstract flag
  data member is set to \a b.
 */

/*! \fn void Node::setWrapper()
  If this node is a ClassNode or a QmlTypeNode, the node's wrapper flag
  data member is set to \c true.
 */

/*! \fn void Node::getMemberNamespaces(NodeMap& out)
  If this is a CollectionNode, \a out is loaded with all the collection
  members that are namespaces.
 */

/*! \fn void getMemberClasses(NodeMap& out) const { }
  If this is a CollectionNode, \a out is loaded with all the collection
  members that are classes.
 */

/*! \fn void Node::setDataType(const QString &dataType)
  If this node is a PropertyNode or a QmlPropertyNode, its
  data type data member is set to \a dataType. Otherwise,
  this function does nothing.
 */

/*! \fn bool Node::wasSeen() const
  Returns the \c seen flag data member of this node if it is a NamespaceNode
  or a CollectionNode. Otherwise it returns \c false. If \c true is returned,
  it means that the location where the namespace or collection is to be
  documented has been found.
 */

/*! \fn void appendGroupName(const QString &t)
  If this node is a PageNode, the group name \a t is appended to the node's
  list of group names. It is not clear to me what this list of group names
  is used for, but it is written to the index file, and it is used in the
  navigation bar.
 */

/*! \fn QString Node::element() const
  If this node is a QmlPropertyNode or a FunctionNode, this function
  returns the name of the parent node. Otherwise it returns an empty
  string.
 */

/*! \fn void Node::setNoAutoList(bool b)
  If this node is a PageNode, the node's \c {no autolist} flag is set to \a b.
  Otherwise the function does nothing.
 */

/*! \fn bool Node::docMustBeGenerated() const
  This function is called to perform a test to decide if the node must have
  documentation generated. In the Node base class, it always returns \c false.

  In the ProxyNode class it always returns \c true. There aren't many proxy
  nodes, but when one appears, it must generate documentation. In the overrides
  in NamespaceNode and ClassNode, a meaningful test is performed to decide if
  documentation must be generated.
 */

/*! \fn QString Node::title() const
  Returns a string that can be used to print a title in the documentation for
  whatever this Node is. In the Node base class, the node's name() is returned.
  In a PageNode, the function returns the title data member. In a HeaderNode,
  if the title() is empty, the name() is returned.
 */

/*! \fn QString Node::subtitle() const { return QString(); }
  Returns a string that can be used to print a subtitle in the documentation for
  whatever this Node is. In the Node base class, the empty string is returned.
  In a PageNode, the function returns the subtitle data member. In a HeaderNode,
  the subtitle data member is returned.
 */

/*! \fn QString Node::fullTitle() const
  Returns a string that can be used as the full title for the documentation of
  this node. In this base class, the name() is returned. In a PageNode, title()
  is returned. In a HeaderNode, if the title() is empty, the name() is returned.
  If the title() is not empty then name-title is returned. In a CollectionNode,
  the title() is returned.
 */

/*! \fn bool Node::setTitle(const QString &title)
  Sets the node's \a title, which is used for the title of
  the documentation page, if one is generated for this node.
  Returns \c true if the title is set. In this base class,
  there is no title string stored, so in the base class,
  nothing happens and \c false is returned. The override in
  the PageNode class is where the title is set.
 */

/*! \fn bool Node::setSubtitle(const QString &subtitle)
  Sets the node's \a subtitle, which is used for the subtitle
  of the documentation page, if one is generated for this node.
  Returns \c true if the subtitle is set. In this base class,
  there is no subtitle string stored, so in the base class,
  nothing happens and \c false is returned. The override in
  the PageNode and HeaderNode classes is where the subtitle is
  set.
 */

/*! \fn void Node::markDefault()
  If this node is a QmlPropertyNode, it is marked as the default property.
  Otherwise the function does nothing.
 */

/*! \fn void Node::markReadOnly(bool flag)
  If this node is a QmlPropertyNode, then the property's read-only
  flag is set to \a flag.
 */

/*! \fn Aggregate *Node::parent() const
  Returns the node's paprent pointer.
*/

/*! \fn const QString &Node::name() const
  Returns the node's name data member.
*/

/*! \fn QString Node::nameForLists() const
  If this node is a PageNode or a HeaderNode, title() is returned.
  Otherwise name() is returned.
 */

/*! \fn QString Node::outputFileName() const
  If this node is a PageNode, the name of the output file that will be
  generated for the node is returned. Otherwise an empty string is returned.
 */

/*! \fn QString Node::obsoleteLink() const
  If this node is a ClassNode or a QmlTypeNode, the link to the obsolete members
  page is returned. Otherwise an empty string is returned.
 */

/*! \fn void Node::setObsoleteLink(const QString &t)
  If this node is a ClassNode or a QmlTypeNode, the link to the obsolete members
  page is set to \a t. Otherwise the function does nothing.
 */

/*! \fn void Node::setQtVariable(const QString &v)
  If this node is a CollectionNode, its QT variable is set to \a v.
  Otherwise the function does nothing. I don't know what the QT variable
  is used for.
 */

/*! \fn QString Node::qtVariable() const
  If this node is a CollectionNode, its QT variable is returned.
  Otherwise an empty string is returned. I don't know what the QT
  variable is used for.
 */

/*! \fn bool Node::hasTag(const QString &t) const
  If this node is a FunctionNode, the function returns \c true if
  the function has the tag \a t. Otherwise the function returns
  \c false. I don't know what the tag is used for.
 */

/*! \fn const QMap<LinkType, QPair<QString, QString> > &Node::links() const
  Returns a reference to this node's link map. The link map should
  probably be moved to the PageNode, because it contains links to the
  start page, next page, previous page, and contents page, and these
  are only used in PageNode, I think.
 */

/*! \fn Access Node::access() const
  Returns the node's Access setting, which can be \c Public,
  \c Protected, or \c Private.
 */

/*! \fn const Location& Node::declLocation() const
  Returns the Location where this node's declaration was seen.
  Normally the declaration location is in an \e include file.
  The declaration location is used in qdoc error/warning messages
  about the declaration.
 */

/*! \fn const Location& Node::defLocation() const
  Returns the Location where this node's dedefinition was seen.
  Normally the definition location is in a \e .cpp file.
  The definition location is used in qdoc error/warning messages
  when the error is discovered at the location of the definition,
  although the way to correct the problem often requires changing
  the declaration.
 */

/*! \fn const Location& Node::location() const
  If this node's definition location is empty, this function
  returns this node's declaration location. Otherwise it
  returns the definition location.

  \sa Location
 */

/*! \fn const Doc &Node::doc() const
  Returns a reference to the node's Doc data member.

  \sa Doc
 */

/*! \fn bool Node::hasDoc() const
  Returns \c true if the node has documentation, i.e. if its Doc
  data member is not empty.

  \sa Doc
 */

/*! \fn Status Node::status() const
  Returns the node's status value.

  \sa Status
 */

/*! \fn QString Node::since() const
  Returns the node's since string, which can be empty.
 */

/*! \fn QString Node::templateStuff() const
  Returns the node's template parameters string, if this node
  represents a templated element.
 */

/*! \fn bool Node::isSharingComment() const
  This function returns \c true if the node is sharing a comment
  with other nodes. For example, multiple functions can be documented
  with a single qdoc comment by listing the \c {\\fn} signatures for
  all the functions in the single qdoc comment.
 */

/*! \fn QString Node::qmlTypeName() const
  If this is a QmlPropertyNode or a FunctionNode representing a QML
  or javascript methos, this function returns the qmlTypeName() of
  the parent() node. Otherwise it returns the name data member.
 */

/*! \fn QString Node::qmlFullBaseName() const
  If this is a QmlTypeNode, this function returns the QML full
  base name. Otherwise it returns an empty string.
 */

/*! \fn QString Node::logicalModuleName() const
  If this is a CollectionNode, this function returns the logical
  module name. Otherwise it returns an empty string.
 */

/*! \fn QString Node::logicalModuleVersion() const
  If this is a CollectionNode, this function returns the logical
  module version number. Otherwise it returns an empty string.
 */

/*! \fn QString Node::logicalModuleIdentifier() const
  If this is a CollectionNode, this function returns the logical
  module identifier. Otherwise it returns an empty string.
 */

/*! \fn void Node::setLogicalModuleInfo(const QString &arg)
  If this node is a CollectionNode, this function splits \a arg
  on the blank character to get a logical module name and version
  number. If the version number is present, it splits the version
  number on the '.' character to get a major version number and a
  minor version number. If the version number is present, both the
  major and minor version numbers should be there, but the minor
  version number is not absolutely necessary.

  The strings are stored in the appropriate data members for use
  when the QML or javascript module page is generated.
 */

/*! \fn void Node::setLogicalModuleInfo(const QStringList &info)
  If this node is a CollectionNode, this function accepts the
  logical module \a info as a string list. If the logical module
  info contains the version number, it splits the version number
  on the '.' character to get the major and minor version numbers.
  Both major and minor version numbers should be provided, but
  the minor version number is not strictly necessary.

  The strings are stored in the appropriate data members for use
  when the QML or javascript module page is generated. This overload
  of the function is called when qdoc is reading an index file.
 */

/*! \fn CollectionNode *Node::logicalModule() const
  If this is a QmlTypeNode, a pointer to its QML module is returned,
  which is a pointer to a CollectionNode. Otherwise the \c nullptr
  is returned.
 */

/*! \fn void Node::setQmlModule(CollectionNode *t)
  If this is a QmlTypeNode, this function sets the QML type's QML module
  pointer to the CollectionNode \a t. Otherwise the function does nothing.
 */

/*! \fn ClassNode *Node::classNode()
  If this is a QmlTypeNode, this function returns the pointer to
  the C++ ClassNode that this QML type represents. Otherwise the
  \c nullptr is returned.
 */

/*! \fn void Node::setClassNode(ClassNode *cn)
  If this is a QmlTypeNode, this function sets the C++ class node
  to \a cn. The C++ ClassNode is the C++ implementation of the QML
  type.
 */

/*! \fn const QString &Node::outputSubdirectory() const
  Returns the node's output subdirector, which is the subdirectory
  of the output directory where the node's documentation file is
  written.
 */

/*! \fn void Node::setOutputSubdirectory(const QString &t)
  Sets the node's output subdirectory to \a t. This is the subdirector
  of the output directory where the node's documentation file will be
  written.
 */

/*! \fn NodeType Node::goal(const QString &t)
  When a square-bracket parameter is used in a qdoc command, this
  function might be called to convert the text string \a t obtained
  from inside the square brackets to be a Goal value, which is returned.

  \sa Goal
 */

/*!
  \class Aggregate
 */

/*! \fn Aggregate::Aggregate(NodeType type, Aggregate *parent, const QString &name)
  The constructor should never be called directly. It is only called
  by the constructors of subclasses of Aggregate. Those constructors
  pass the node \a type they want to create, the \a parent of the new
  node, and its \a name.
 */

/*!
  The destructor calls delete for each child of this Aggregate
  that has this Aggregate as its parent. A child node that has
  some other Aggregate as its parent is deleted by that
  Aggregate's destructor. This is a fail-safe test.

  The destructor no longer deletes the collection of children
  by calling qDeleteAll() because the child list can contain
  pointers to children that have some other Aggregate as their
  parent. This is because of how the \e{\\relates} command is
  processed. An Aggregate can have a pointer to, for example,
  a FunctionNode in its child list, but that FunctionNode has
  a differen Aggregate as its parent because a \e{\\relates}
  command was used to relate it to that parent. In that case,
  the other Aggregate's destructor must delete that node.

  \note This function is the \b only place where delete is
  called to delete any subclass of Node.

  \note This strategy depends on the node tree being destroyed
  by calling delete on the root node of the tree. This happens
  in the destructor of class Tree.
 */
Aggregate::~Aggregate()
{
    enumChildren_.clear();
    nonfunctionMap_.clear();
    functionMap_.clear();
    for (int i = 0; i < children_.size(); ++i) {
        if ((children_[i] != nullptr) && (children_[i]->parent() == this))
            delete children_[i];
        children_[i] = nullptr;
    }
    children_.clear();
}

/*!
  If \a genus is \c{Node::DontCare}, find the first node in
  this node's child list that has the given \a name. If this
  node is a QML type, be sure to also look in the children
  of its property group nodes. Return the matching node or 0.

  If \a genus is either \c{Node::CPP} or \c {Node::QML}, then
  find all this node's children that have the given \a name,
  and return the one that satisfies the \a genus requirement.
 */
Node *Aggregate::findChildNode(const QString &name, Node::Genus genus, int findFlags) const
{
    if (genus == Node::DontCare) {
        Node *node = nonfunctionMap_.value(name);
        if (node)
            return node;
    } else {
        NodeList nodes = nonfunctionMap_.values(name);
        if (!nodes.isEmpty()) {
            for (int i = 0; i < nodes.size(); ++i) {
                Node *node = nodes.at(i);
                if (genus == node->genus()) {
                    if (findFlags & TypesOnly) {
                        if (!node->isTypedef() && !node->isClassNode() && !node->isQmlType()
                            && !node->isQmlBasicType() && !node->isJsType()
                            && !node->isJsBasicType() && !node->isEnumType())
                            continue;
                    } else if (findFlags & IgnoreModules && node->isModule())
                        continue;
                    return node;
                }
            }
        }
    }
    if (genus != Node::DontCare && this->genus() != genus)
        return nullptr;
    return functionMap_.value(name);
}

/*!
  Find all the child nodes of this node that are named
  \a name and return them in \a nodes.
 */
void Aggregate::findChildren(const QString &name, NodeVector &nodes) const
{
    nodes.clear();
    int nonfunctionCount = nonfunctionMap_.count(name);
    auto it = functionMap_.find(name);
    if (it != functionMap_.end()) {
        int functionCount = 0;
        FunctionNode *fn = it.value();
        while (fn != nullptr) {
            ++functionCount;
            fn = fn->nextOverload();
        }
        nodes.reserve(nonfunctionCount + functionCount);
        fn = it.value();
        while (fn != nullptr) {
            nodes.append(fn);
            fn = fn->nextOverload();
        }
    } else {
        nodes.reserve(nonfunctionCount);
    }
    if (nonfunctionCount > 0) {
        for (auto it = nonfunctionMap_.find(name); it != nonfunctionMap_.end() && it.key() == name;
             ++it) {
            nodes.append(it.value());
        }
    }
}

/*!
  This function searches for a child node of this Aggregate,
  such that the child node has the spacified \a name and the
  function \a isMatch returns true for the node. The function
  passed must be one of the isXxx() functions in class Node
  that tests the node type.
 */
Node *Aggregate::findNonfunctionChild(const QString &name, bool (Node::*isMatch)() const)
{
    NodeList nodes = nonfunctionMap_.values(name);
    for (int i = 0; i < nodes.size(); ++i) {
        Node *node = nodes.at(i);
        if ((node->*(isMatch))())
            return node;
    }
    return nullptr;
}

/*!
  Find a function node that is a child of this node, such that
  the function node has the specified \a name and \a parameters.
  If \a parameters is empty but no matching function is found
  that has no parameters, return the first non-internal primary
  function or overload, whether it has parameters or not.
 */
FunctionNode *Aggregate::findFunctionChild(const QString &name, const Parameters &parameters)
{
    auto it = functionMap_.find(name);
    if (it == functionMap_.end())
        return nullptr;
    FunctionNode *fn = it.value();

    if (parameters.isEmpty() && fn->parameters().isEmpty() && !fn->isInternal())
        return fn;

    while (fn != nullptr) {
        if (parameters.count() == fn->parameters().count() && !fn->isInternal()) {
            if (parameters.isEmpty())
                return fn;
            bool matched = true;
            for (int i = 0; i < parameters.count(); i++) {
                if (parameters.at(i).type() != fn->parameters().at(i).type()) {
                    matched = false;
                    break;
                }
            }
            if (matched)
                return fn;
        }
        fn = fn->nextOverload();
    }

    if (parameters.isEmpty()) {
        for (fn = it.value(); fn != nullptr; fn = fn->nextOverload())
            if (!fn->isInternal())
                return fn;
        return it.value();
    }
    return nullptr;
}

/*!
  Find the function node that is a child of this node, such
  that the function described has the same name and signature
  as the function described by the function node \a clone.
 */
FunctionNode *Aggregate::findFunctionChild(const FunctionNode *clone)
{
    FunctionNode *fn = functionMap_.value(clone->name());
    while (fn != nullptr) {
        if (isSameSignature(clone, fn))
            return fn;
        fn = fn->nextOverload();
    }
    return nullptr;
}

/*!
  Returns the list of keys from the primary function map.
 */
QStringList Aggregate::primaryKeys()
{
    return functionMap_.keys();
}

/*!
  Mark all child nodes that have no documentation as having
  private access and internal status. qdoc will then ignore
  them for documentation purposes.
 */
void Aggregate::markUndocumentedChildrenInternal()
{
    for (auto *child : qAsConst(children_)) {
        if (!child->isSharingComment() && !child->hasDoc() && !child->isDontDocument()) {
            if (!child->docMustBeGenerated()) {
                if (child->isFunction()) {
                    if (static_cast<FunctionNode *>(child)->hasAssociatedProperties())
                        continue;
                } else if (child->isTypedef()) {
                    if (static_cast<TypedefNode *>(child)->hasAssociatedEnum())
                        continue;
                }
                child->setAccess(Access::Private);
                child->setStatus(Node::Internal);
            }
        }
        if (child->isAggregate()) {
            static_cast<Aggregate *>(child)->markUndocumentedChildrenInternal();
        }
    }
}

/*!
  This is where we set the overload numbers for function nodes.
  \note Overload numbers for related non-members are handled
  separately.
 */
void Aggregate::normalizeOverloads()
{
    /*
      Ensure that none of the primary functions is inactive, private,
      or marked \e {overload}.
    */
    for (auto it = functionMap_.begin(); it != functionMap_.end(); ++it) {
        FunctionNode *fn = it.value();
        if (fn->isOverload()) {
            FunctionNode *primary = fn->findPrimaryFunction();
            if (primary) {
                primary->setNextOverload(fn);
                it.value() = primary;
                fn = primary;
            } else {
                fn->clearOverloadFlag();
            }
        }
        int count = 0;
        fn->setOverloadNumber(0);
        FunctionNode *internalFn = nullptr;
        while (fn != nullptr) {
            FunctionNode *next = fn->nextOverload();
            if (next) {
                if (next->isInternal()) {
                    // internal overloads are moved to a separate list
                    // and processed last
                    fn->setNextOverload(next->nextOverload());
                    next->setNextOverload(internalFn);
                    internalFn = next;
                } else {
                    next->setOverloadNumber(++count);
                }
                fn = fn->nextOverload();
            } else {
                fn->setNextOverload(internalFn);
                break;
            }
        }
        while (internalFn) {
            internalFn->setOverloadNumber(++count);
            internalFn = internalFn->nextOverload();
        }
        // process next function in function map.
    }
    /*
      Recursive part.
     */
    for (auto *node : qAsConst(children_)) {
        if (node->isAggregate())
            static_cast<Aggregate *>(node)->normalizeOverloads();
    }
}

/*!
  Returns a const reference to the list of child nodes of this
  aggregate that are not function nodes. Duplicate nodes are
  removed from the list.
 */
const NodeList &Aggregate::nonfunctionList()
{
    nonfunctionList_ = nonfunctionMap_.values();
    std::sort(nonfunctionList_.begin(), nonfunctionList_.end(), Node::nodeNameLessThan);
    nonfunctionList_.erase(std::unique(nonfunctionList_.begin(), nonfunctionList_.end()),
                           nonfunctionList_.end());
    return nonfunctionList_;
}

/*! \fn bool Aggregate::isAggregate() const
  Returns \c true because this node is an instance of Aggregate,
  which means it can have children.
 */

/*!
  Finds the enum type node that has \a enumValue as one of
  its enum values and returns a pointer to it. Returns 0 if
  no enum type node is found that has \a enumValue as one
  of its values.
 */
const EnumNode *Aggregate::findEnumNodeForValue(const QString &enumValue) const
{
    for (const auto *node : enumChildren_) {
        const EnumNode *en = static_cast<const EnumNode *>(node);
        if (en->hasItem(enumValue))
            return en;
    }
    return nullptr;
}

/*!
  Appends \a includeFile file to the list of include files.
 */
void Aggregate::addIncludeFile(const QString &includeFile)
{
    includeFiles_.append(includeFile);
}

/*!
  Sets the list of include files to \a includeFiles.
 */
void Aggregate::setIncludeFiles(const QStringList &includeFiles)
{
    includeFiles_ = includeFiles;
}

/*!
  Compare \a f1 to \a f2 and return \c true if they have the same
  signature. Otherwise return \c false. They must have the same
  number of parameters, and all the parameter types must be the
  same. The functions must have the same constness and refness.
  This is a private function.
 */
bool Aggregate::isSameSignature(const FunctionNode *f1, const FunctionNode *f2)
{
    if (f1->parameters().count() != f2->parameters().count())
        return false;
    if (f1->isConst() != f2->isConst())
        return false;
    if (f1->isRef() != f2->isRef())
        return false;
    if (f1->isRefRef() != f2->isRefRef())
        return false;

    const Parameters &p1 = f1->parameters();
    const Parameters &p2 = f2->parameters();
    for (int i = 0; i < p1.count(); i++) {
        if (p1.at(i).hasType() && p2.at(i).hasType()) {
            QString t1 = p1.at(i).type();
            QString t2 = p2.at(i).type();

            if (t1.length() < t2.length())
                qSwap(t1, t2);

            /*
              ### hack for C++ to handle superfluous
              "Foo::" prefixes gracefully
             */
            if (t1 != t2 && t1 != (f2->parent()->name() + "::" + t2)) {
                // Accept a difference in the template parametters of the type if one
                // is omited (eg. "QAtomicInteger" == "QAtomicInteger<T>")
                auto ltLoc = t1.indexOf('<');
                auto gtLoc = t1.indexOf('>', ltLoc);
                if (ltLoc < 0 || gtLoc < ltLoc)
                    return false;
                t1.remove(ltLoc, gtLoc - ltLoc + 1);
                if (t1 != t2)
                    return false;
            }
        }
    }
    return true;
}

/*!
  This function is only called by addChild(), when the child is a
  FunctionNode. If the function map does not contain a function with
  the name in \a fn, \a fn is inserted into the function map. If the
  map already contains a function by that name, \a fn is appended to
  that function's linked list of overloads.

  \note A function's overloads appear in the linked list in the same
  order they were created. The first overload in the list is the first
  overload created. This order is maintained in the numbering of
  overloads. In other words, the first overload in the linked list has
  overload number 1, and the last overload in the list has overload
  number n, where n is the number of overloads not including the
  function in the function map.

  \not Adding a function increments the aggregate's function count,
  which is the total number of function nodes in the function map,
  including the overloads. The overloads are not inserted into the map
  but are in a linked list using the FunctionNode's m_nextOverload
  pointer.

  \note The function's overload number and overload flag are set in
  normalizeOverloads().

  \note This is a private function.

  \sa normalizeOverloads()
 */
void Aggregate::addFunction(FunctionNode *fn)
{
    auto it = functionMap_.find(fn->name());
    if (it == functionMap_.end())
        functionMap_.insert(fn->name(), fn);
    else
        it.value()->appendOverload(fn);
    functionCount_++;
}

/*!
  When an Aggregate adopts a function that is a child of
  another Aggregate, the function is inserted into this
  Aggregate's function map, if the function's name is not
  already in the function map. If the function's name is
  already in the function map, do nothing. The overload
  link is already set correctly.

  \note This is a private function.
 */
void Aggregate::adoptFunction(FunctionNode *fn)
{
    auto it = functionMap_.find(fn->name());
    if (it == functionMap_.end())
        functionMap_.insert(fn->name(), fn);
    ++functionCount_;
}

/*!
  Adds the \a child to this node's child map using \a title
  as the key. The \a child is not added to the child list
  again, because it is presumed to already be there. We just
  want to be able to find the child by its \a title.
 */
void Aggregate::addChildByTitle(Node *child, const QString &title)
{
    nonfunctionMap_.insert(title, child);
}

/*!
  Adds the \a child to this node's child list and sets the child's
  parent pointer to this Aggregate. It then mounts the child with
  mountChild().

  The \a child is then added to this Aggregate's searchable maps
  and lists.

  \note This function does not test the child's parent pointer
  for null before changing it. If the child's parent pointer
  is not null, then it is being reparented. The child becomes
  a child of this Aggregate, but it also remains a child of
  the Aggregate that is it's old parent. But the child will
  only have one parent, and it will be this Aggregate. The is
  because of the \c relates command.

  \sa mountChild(), dismountChild()
 */
void Aggregate::addChild(Node *child)
{
    children_.append(child);
    child->setParent(this);
    child->setOutputSubdirectory(this->outputSubdirectory());
    child->setUrl(QString());
    child->setIndexNodeFlag(isIndexNode());

    if (child->isFunction()) {
        addFunction(static_cast<FunctionNode *>(child));
    } else if (!child->name().isEmpty()) {
        nonfunctionMap_.insert(child->name(), child);
        if (child->isEnumType())
            enumChildren_.append(child);
    }
}

/*!
  This Aggregate becomes the adoptive parent of \a child. The
  \a child knows this Aggregate as its parent, but its former
  parent continues to have pointers to the child in its child
  list and in its searchable data structures. But the child is
  also added to the child list and searchable data structures
  of this Aggregate.

  The one caveat is that if the child being adopted is a function
  node, it's next overload pointer is not altered.
 */
void Aggregate::adoptChild(Node *child)
{
    if (child->parent() != this) {
        children_.append(child);
        child->setParent(this);
        if (child->isFunction()) {
            adoptFunction(static_cast<FunctionNode *>(child));
        } else if (!child->name().isEmpty()) {
            nonfunctionMap_.insert(child->name(), child);
            if (child->isEnumType())
                enumChildren_.append(child);
        }
        if (child->isSharedCommentNode()) {
            SharedCommentNode *scn = static_cast<SharedCommentNode *>(child);
            for (Node *n : scn->collective())
                adoptChild(n);
        }
    }
}

/*!
  Recursively sets the output subdirectory for children
 */
void Aggregate::setOutputSubdirectory(const QString &t)
{
    Node::setOutputSubdirectory(t);
    for (auto *node : qAsConst(children_))
        node->setOutputSubdirectory(t);
}

/*!
  If this node has a child that is a QML property or JS property
  named \a n, return a pointer to that child. Otherwise, return \nullptr.
 */
QmlPropertyNode *Aggregate::hasQmlProperty(const QString &n) const
{
    NodeType goal = Node::QmlProperty;
    if (isJsNode())
        goal = Node::JsProperty;
    for (auto *child : qAsConst(children_)) {
        if (child->nodeType() == goal) {
            if (child->name() == n)
                return static_cast<QmlPropertyNode *>(child);
        }
    }
    return nullptr;
}

/*!
  If this node has a child that is a QML property or JS property
  named \a n and that also matches \a attached, return a pointer
  to that child.
 */
QmlPropertyNode *Aggregate::hasQmlProperty(const QString &n, bool attached) const
{
    NodeType goal = Node::QmlProperty;
    if (isJsNode())
        goal = Node::JsProperty;
    for (auto *child : qAsConst(children_)) {
        if (child->nodeType() == goal) {
            if (child->name() == n && child->isAttached() == attached)
                return static_cast<QmlPropertyNode *>(child);
        }
    }
    return nullptr;
}

/*!
  The FunctionNode \a fn is assumed to be a member function
  of this Aggregate. The function's name is looked up in the
  Aggregate's function map. It should be found because it is
  assumed that \a fn is in this Aggregate's function map. But
  in case it is not found, \c false is returned.

  Normally, the name will be found in the function map, and
  the value of the iterator is used to get the value, which
  is a pointer to another FunctionNode, which is not itself
  an overload. If that function has a non-null overload
  pointer, true is returned. Otherwise false is returned.

  This is a convenience function that you should not need to
  use.
 */
bool Aggregate::hasOverloads(const FunctionNode *fn) const
{
    auto it = functionMap_.find(fn->name());
    return (it == functionMap_.end() ? false : (it.value()->nextOverload() != nullptr));
}

/*!
  Prints the inner node's list of children.
  For debugging only.
 */
void Aggregate::printChildren(const QString &title)
{
    qDebug() << title << name() << children_.size();
    if (children_.size() > 0) {
        for (int i = 0; i < children_.size(); ++i) {
            Node *n = children_.at(i);
            qDebug() << "  CHILD:" << n->name() << n->nodeTypeString();
        }
    }
}

/*!
  Removes \a fn from this aggregate's function map. That's
  all it does. If \a fn is in the function map index and it
  has an overload, the value pointer in the function map
  index is set to the the overload pointer. If the function
  has no overload pointer, the function map entry is erased.

  \note When removing a function node from the function map,
  it is important to set the removed function node's next
  overload pointer to null because the function node might
  be added as a child to some other aggregate.

  \note This is a protected function.
 */
void Aggregate::removeFunctionNode(FunctionNode *fn)
{
    auto it = functionMap_.find(fn->name());
    if (it != functionMap_.end()) {
        if (it.value() == fn) {
            if (fn->nextOverload() != nullptr) {
                it.value() = fn->nextOverload();
                fn->setNextOverload(nullptr);
                fn->setOverloadNumber(0);
            } else {
                functionMap_.erase(it);
            }
        } else {
            FunctionNode *current = it.value();
            while (current != nullptr) {
                if (current->nextOverload() == fn) {
                    current->setNextOverload(fn->nextOverload());
                    fn->setNextOverload(nullptr);
                    fn->setOverloadNumber(0);
                    break;
                }
                current = current->nextOverload();
            }
        }
    }
}

/*
  When deciding whether to include a function in the function
  index, if the function is marked private, don't include it.
  If the function is marked obsolete, don't include it. If the
  function is marked internal, don't include it. Or if the
  function is a destructor or any kind of constructor, don't
  include it. Otherwise include it.
 */
static bool keep(FunctionNode *fn)
{
    if (fn->isPrivate() || fn->isObsolete() || fn->isInternal() || fn->isSomeCtor() || fn->isDtor())
        return false;
    return true;
}

/*!
  Insert all functions declared in this aggregate into the
  \a functionIndex. Call the function recursively for each
  child that is an aggregate.

  Only include functions that are in the public API and
  that are not constructors or destructors.
 */
void Aggregate::findAllFunctions(NodeMapMap &functionIndex)
{
    for (auto it = functionMap_.constBegin(); it != functionMap_.constEnd(); ++it) {
        FunctionNode *fn = it.value();
        if (keep(fn))
            functionIndex[fn->name()].insert(fn->parent()->fullDocumentName(), fn);
        fn = fn->nextOverload();
        while (fn != nullptr) {
            if (keep(fn))
                functionIndex[fn->name()].insert(fn->parent()->fullDocumentName(), fn);
            fn = fn->nextOverload();
        }
    }
    for (Node *node : qAsConst(children_)) {
        if (node->isAggregate() && !node->isPrivate())
            static_cast<Aggregate *>(node)->findAllFunctions(functionIndex);
    }
}

/*!
  For each child of this node, if the child is a namespace node,
  insert the child into the \a namespaces multimap. If the child
  is an aggregate, call this function recursively for that child.

  When the function called with the root node of a tree, it finds
  all the namespace nodes in that tree and inserts them into the
  \a namespaces multimap.

  The root node of a tree is a namespace, but it has no name, so
  it is not inserted into the map. So, if this function is called
  for each tree in the qdoc database, it finds all the namespace
  nodes in the database.
  */
void Aggregate::findAllNamespaces(NodeMultiMap &namespaces)
{
    for (auto *node : qAsConst(children_)) {
        if (node->isAggregate() && !node->isPrivate()) {
            if (node->isNamespace() && !node->name().isEmpty())
                namespaces.insert(node->name(), node);
            static_cast<Aggregate *>(node)->findAllNamespaces(namespaces);
        }
    }
}

/*!
  Returns true if this aggregate contains at least one child
  that is marked obsolete. Otherwise returns false.
 */
bool Aggregate::hasObsoleteMembers() const
{
    for (const auto *node : children_) {
        if (!node->isPrivate() && node->isObsolete()) {
            if (node->isFunction() || node->isProperty() || node->isEnumType() || node->isTypedef()
                || node->isTypeAlias() || node->isVariable() || node->isQmlProperty()
                || node->isJsProperty())
                return true;
        }
    }
    return false;
}

/*!
  Finds all the obsolete C++ classes and QML types in this
  aggregate and all the C++ classes and QML types with obsolete
  members, and inserts them into maps used elsewhere for
  generating documentation.
 */
void Aggregate::findAllObsoleteThings()
{
    for (auto *node : qAsConst(children_)) {
        if (!node->isPrivate()) {
            QString name = node->name();
            if (node->isObsolete()) {
                if (node->isClassNode())
                    QDocDatabase::obsoleteClasses().insert(node->qualifyCppName(), node);
                else if (node->isQmlType() || node->isJsType())
                    QDocDatabase::obsoleteQmlTypes().insert(node->qualifyQmlName(), node);
            } else if (node->isClassNode()) {
                Aggregate *a = static_cast<Aggregate *>(node);
                if (a->hasObsoleteMembers())
                    QDocDatabase::classesWithObsoleteMembers().insert(node->qualifyCppName(), node);
            } else if (node->isQmlType() || node->isJsType()) {
                Aggregate *a = static_cast<Aggregate *>(node);
                if (a->hasObsoleteMembers())
                    QDocDatabase::qmlTypesWithObsoleteMembers().insert(node->qualifyQmlName(),
                                                                       node);
            } else if (node->isAggregate()) {
                static_cast<Aggregate *>(node)->findAllObsoleteThings();
            }
        }
    }
}

/*!
  Finds all the C++ classes, QML types, JS types, QML and JS
  basic types, and examples in this aggregate and inserts them
  into appropriate maps for later use in generating documentation.
 */
void Aggregate::findAllClasses()
{
    for (auto *node : qAsConst(children_)) {
        if (!node->isPrivate() && !node->isInternal() && !node->isDontDocument()
            && node->tree()->camelCaseModuleName() != QString("QDoc")) {
            if (node->isClassNode()) {
                QDocDatabase::cppClasses().insert(node->qualifyCppName().toLower(), node);
            } else if (node->isQmlType() || node->isQmlBasicType() || node->isJsType()
                       || node->isJsBasicType()) {
                QString name = node->unqualifyQmlName();
                QDocDatabase::qmlTypes().insert(name, node);
                // also add to the QML basic type map
                if (node->isQmlBasicType() || node->isJsBasicType())
                    QDocDatabase::qmlBasicTypes().insert(name, node);
            } else if (node->isExample()) {
                // use the module index title as key for the example map
                QString title = node->tree()->indexTitle();
                if (!QDocDatabase::examples().contains(title, node))
                    QDocDatabase::examples().insert(title, node);
            } else if (node->isAggregate()) {
                static_cast<Aggregate *>(node)->findAllClasses();
            }
        }
    }
}

/*!
  Find all the attribution pages in this node and insert them
  into \a attributions.
 */
void Aggregate::findAllAttributions(NodeMultiMap &attributions)
{
    for (auto *node : qAsConst(children_)) {
        if (!node->isPrivate()) {
            if (node->pageType() == Node::AttributionPage)
                attributions.insert(node->tree()->indexTitle(), node);
            else if (node->isAggregate())
                static_cast<Aggregate *>(node)->findAllAttributions(attributions);
        }
    }
}

/*!
  Finds all the nodes in this node where a \e{since} command appeared
  in the qdoc comment and sorts them into maps according to the kind
  of node.

  This function is used for generating the "New Classes... in x.y"
  section on the \e{What's New in Qt x.y} page.
 */
void Aggregate::findAllSince()
{
    for (auto *node : qAsConst(children_)) {
        QString sinceString = node->since();
        // Insert a new entry into each map for each new since string found.
        if (!node->isPrivate() && !sinceString.isEmpty()) {
            auto nsmap = QDocDatabase::newSinceMaps().find(sinceString);
            if (nsmap == QDocDatabase::newSinceMaps().end())
                nsmap = QDocDatabase::newSinceMaps().insert(sinceString, NodeMultiMap());

            auto ncmap = QDocDatabase::newClassMaps().find(sinceString);
            if (ncmap == QDocDatabase::newClassMaps().end())
                ncmap = QDocDatabase::newClassMaps().insert(sinceString, NodeMap());

            auto nqcmap = QDocDatabase::newQmlTypeMaps().find(sinceString);
            if (nqcmap == QDocDatabase::newQmlTypeMaps().end())
                nqcmap = QDocDatabase::newQmlTypeMaps().insert(sinceString, NodeMap());

            if (node->isFunction()) {
                // Insert functions into the general since map.
                FunctionNode *fn = static_cast<FunctionNode *>(node);
                if (!fn->isObsolete() && !fn->isSomeCtor() && !fn->isDtor())
                    nsmap.value().insert(fn->name(), fn);
            } else if (node->isClassNode()) {
                // Insert classes into the since and class maps.
                QString name = node->qualifyWithParentName();
                nsmap.value().insert(name, node);
                ncmap.value().insert(name, node);
            } else if (node->isQmlType() || node->isJsType()) {
                // Insert QML elements into the since and element maps.
                QString name = node->qualifyWithParentName();
                nsmap.value().insert(name, node);
                nqcmap.value().insert(name, node);
            } else if (node->isQmlProperty() || node->isJsProperty()) {
                // Insert QML properties into the since map.
                nsmap.value().insert(node->name(), node);
            } else {
                // Insert external documents into the general since map.
                QString name = node->qualifyWithParentName();
                nsmap.value().insert(name, node);
            }
        }
        // Recursively find child nodes with since commands.
        if (node->isAggregate())
            static_cast<Aggregate *>(node)->findAllSince();
    }
}

/*!
  For each QML Type node in this aggregate's children, if the
  QML type has a QML base type name but its QML base type node
  pointer is nullptr, use the QML base type name to look up the
  base type node. If the node is found, set the node's QML base
  type node pointer to that node.
 */
void Aggregate::resolveQmlInheritance()
{
    NodeMap previousSearches;
    // Do we need recursion?
    for (auto *child : qAsConst(children_)) {
        if (!child->isQmlType() && !child->isJsType())
            continue;
        QmlTypeNode *type = static_cast<QmlTypeNode *>(child);
        if (type->qmlBaseNode() != nullptr)
            continue;
        if (type->qmlBaseName().isEmpty())
            continue;
        QmlTypeNode *base = static_cast<QmlTypeNode *>(previousSearches.value(type->qmlBaseName()));
        if (base && (base != type)) {
            type->setQmlBaseNode(base);
            QmlTypeNode::addInheritedBy(base, type);
        } else {
            if (!type->importList().isEmpty()) {
                const ImportList &imports = type->importList();
                for (int i = 0; i < imports.size(); ++i) {
                    base = QDocDatabase::qdocDB()->findQmlType(imports[i], type->qmlBaseName());
                    if (base && (base != type)) {
                        if (base->logicalModuleVersion()[0] != imports[i].m_majorMinorVersion[0])
                            base = nullptr; // Safeguard for QTBUG-53529
                        break;
                    }
                }
            }
            if (base == nullptr) {
                base = QDocDatabase::qdocDB()->findQmlType(QString(), type->qmlBaseName());
            }
            if (base && (base != type)) {
                type->setQmlBaseNode(base);
                QmlTypeNode::addInheritedBy(base, type);
                previousSearches.insert(type->qmlBaseName(), base);
            }
        }
    }
}

/*!
  Returns a word representing the kind of Aggregate this node is.
  Currently only works for class, struct, and union, but it can
  easily be extended. If \a cap is true, the word is capitalised.
 */
QString Aggregate::typeWord(bool cap) const
{
    if (cap) {
        switch (nodeType()) {
        case Node::Class:
            return QLatin1String("Class");
        case Node::Struct:
            return QLatin1String("Struct");
        case Node::Union:
            return QLatin1String("Union");
        default:
            break;
        }
    } else {
        switch (nodeType()) {
        case Node::Class:
            return QLatin1String("class");
        case Node::Struct:
            return QLatin1String("struct");
        case Node::Union:
            return QLatin1String("union");
        default:
            break;
        }
    }
    return QString();
}

/*! \fn int Aggregate::count() const
  Returns the number of children in the child list.
 */

/*! \fn const NodeList &Aggregate::childNodes() const
  Returns a const reference to the child list.
 */

/*! \fn NodeList::ConstIterator Aggregate::constBegin() const
  Returns a const iterator pointing at the beginning of the child list.
 */

/*! \fn NodeList::ConstIterator Aggregate::constEnd() const
  Returns a const iterator pointing at the end of the child list.
 */

/*! \fn const QStringList &Aggregate::includeFiles() const
  This function returns a const reference to a list of strings, but
  I no longer know what they are.
 */

/*! \fn QmlTypeNode *Aggregate::qmlBaseNode() const
  If this Aggregate is a QmlTypeNode, this function returns a pointer to
  the QmlTypeNode that is its base type. Otherwise it returns \c nullptr.
  A QmlTypeNode doesn't always have a base type, so even when this Aggregate
  is aQmlTypeNode, the pointer returned can be \c nullptr.
 */

/*! \fn FunctionMap &Aggregate::functionMap()
  Returns a reference to this Aggregate's function map, which
  is a map of all the children of this Aggregate that are
  FunctionNodes.
 */

/*! \fn void Aggregate::appendToRelatedByProxy(const NodeList &t)
  Appends the list of node pointers to the list of elements that are
  related to this Aggregate but are documented in a different module.

  \sa relatedByProxy()
 */

/*! \fn NodeList &Aggregate::relatedByProxy()
  Returns a reference to a list of node pointers where each element
  points to a node in an index file for some other module, such that
  whatever the node represents was documented in that other module,
  but it is related to this Aggregate, so when the documentation for
  this Aggregate is written, it will contain links to elements in the
  other module.
 */

/*!
  \class PageNode
  \brief A PageNode is a Node that generates a documentation page.

  Not all subclasses of Node produce documentation pages. FunctionNode,
  PropertyNode, and EnumNode are all examples of subclasses of Node that
  don't produce documentation pages but add documentation to a page.
  They are always child nodes of an Aggregate, and Aggregate inherits
  PageNode.

  Not every subclass of PageNode inherits Aggregate. ExternalPageNode,
  ExampleNode, and CollectionNode are subclasses of PageNode that are
  not subclasses of Aggregate. Because they are not subclasses of
  Aggregate, they can't have children. But they still generate, or
  link to, a documentation page.
 */

/*! \fn QString PageNode::title() const
  Returns the node's title, which is used for the page title.
 */

/*! \fn QString PageNode::subtitle() const
  Returns the node's subtitle, which may be empty.
 */

/*!
  Returns the node's full title.
 */
QString PageNode::fullTitle() const
{
    return title();
}

/*!
  Sets the node's \a title, which is used for the page title.
  Returns true. Adds the node to the parent() nonfunction map
  using the \a title as the key.
 */
bool PageNode::setTitle(const QString &title)
{
    title_ = title;
    parent()->addChildByTitle(this, title);
    return true;
}

/*!
  \fn bool PageNode::setSubtitle(const QString &subtitle)
  Sets the node's \a subtitle. Returns true;
 */

/*! \fn PageNode::PageNode(Aggregate *parent, const QString &name)
  This constructor sets the PageNode's \a parent and the \a name is the
  argument of the \c {\\page} command. The node type is set to Node::Page.
 */

/*! \fn PageNode::PageNode(NodeType type, Aggregate *parent, const QString &name)
  This constructor is not called directly. It is called by the constructors of
  subclasses of PageNode, usually Aggregate. The node type is set to \a type,
  and the parent pointer is set to \a parent. \a name is the argument of the topic
  command that resulted in the PageNode being created. This could be \c {\\class}
  or \c {\\namespace}, for example.
 */

/*! \fn PageNode::PageNode(Aggregate *parent, const QString &name, PageType ptype)
  This constructor is called when the argument to the \c {\\page} command
  contains a space, which means the second word of the argument is the page type.
  It creates a PageNode that has node type Node::Page, with the specified
  \a parent and \name, and the \a ptype is that second word of the argument to
  the \c {\\page} command.

  \sa Node::PageType
 */

/*! \fn PageNode::~PageNode()
  The destructor is virtual, and it does nothing.
 */

/*! \fn bool PageNode::isPageNode() const
  Always returns \c true because this is a PageNode.
 */

/*! \fn bool PageNode::isTextPageNode() const
  Returns \c true if this instance of PageNode is not an Aggregate.
  The significance of a \c true return value is that this PageNode
  doesn't have children, because it is not an Aggregate.

  \sa Aggregate.
 */

/*! \fn QString PageNode::nameForLists() const
  Returns the title().
 */

/*! \fn QString PageNode::imageFileName() const
  If this PageNode is an ExampleNode, the image file name
  data member is returned. Otherwise an empty string is
  returned.
 */

/*! \fn void PageNode::setImageFileName(const QString &ifn)
  If this PageNode is an ExampleNode, the image file name
  data member is set to \a ifn. Otherwise the function does
  nothing.
 */

/*! \fn bool PageNode::noAutoList() const
  Returns the value of the no auto-list flag.
 */

/*! \fn void PageNode::setNoAutoList(bool b)
  Sets the no auto-list flag to \a b.
 */

/*! \fn const QStringList &PageNode::groupNames() const
  Returns a const reference to the string list containing all the group names.
 */

/*! \fn void PageNode::appendGroupName(const QString &t)
  Appends \a t to the list of group names.
 */

/*! \fn void PageNode::setOutputFileName(const QString &f)
  Sets this PageNode's output file name to \a f.
 */

/*! \fn QString PageNode::outputFileName() const
  Returns this PageNode's output file name.
 */

QT_END_NAMESPACE
