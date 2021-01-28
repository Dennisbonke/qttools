/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
****************************************************************************/

/*
  text.h
*/

#ifndef TEXT_H
#define TEXT_H

#include "atom.h"

QT_BEGIN_NAMESPACE

class Text
{
public:
    Text();
    explicit Text(const QString &str);
    Text(const Text &text);
    ~Text();

    Text &operator=(const Text &text);

    Atom *firstAtom() { return first; }
    Atom *lastAtom() { return last; }
    Text &operator<<(Atom::AtomType atomType);
    Text &operator<<(const QString &string);
    Text &operator<<(const Atom &atom);
    Text &operator<<(const LinkAtom &atom);
    Text &operator<<(const Text &text);
    void stripFirstAtom();
    void stripLastAtom();

    bool isEmpty() const { return first == nullptr; }
    bool contains(const QString &str) const;
    QString toString() const;
    const Atom *firstAtom() const { return first; }
    const Atom *lastAtom() const { return last; }
    Text subText(Atom::AtomType left, Atom::AtomType right, const Atom *from = nullptr,
                 bool inclusive = false) const;
    void dump() const;
    void clear();

    static Text subText(const Atom *begin, const Atom *end = nullptr);
    static Text sectionHeading(const Atom *sectionBegin);
    static const Atom *sectionHeadingAtom(const Atom *sectionLeft);
    static int compare(const Text &text1, const Text &text2);

private:
    Atom *first;
    Atom *last;
};

inline bool operator==(const Text &text1, const Text &text2)
{
    return Text::compare(text1, text2) == 0;
}
inline bool operator!=(const Text &text1, const Text &text2)
{
    return Text::compare(text1, text2) != 0;
}
inline bool operator<(const Text &text1, const Text &text2)
{
    return Text::compare(text1, text2) < 0;
}
inline bool operator<=(const Text &text1, const Text &text2)
{
    return Text::compare(text1, text2) <= 0;
}
inline bool operator>(const Text &text1, const Text &text2)
{
    return Text::compare(text1, text2) > 0;
}
inline bool operator>=(const Text &text1, const Text &text2)
{
    return Text::compare(text1, text2) >= 0;
}

QT_END_NAMESPACE

#endif
