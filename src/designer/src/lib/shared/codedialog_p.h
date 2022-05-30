/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Designer of the Qt Toolkit.
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
******************************************************************************/

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef CODEPREVIEWDIALOG_H
#define CODEPREVIEWDIALOG_H

#include "shared_global_p.h"
#include <QtWidgets/qdialog.h>

QT_BEGIN_NAMESPACE

class QDesignerFormWindowInterface;

namespace qdesigner_internal {

enum class UicLanguage;

// Dialog for viewing code.
class QDESIGNER_SHARED_EXPORT CodeDialog : public QDialog
{
    Q_OBJECT
    explicit CodeDialog(QWidget *parent = nullptr);
public:
    ~CodeDialog() override;

    static bool generateCode(const QDesignerFormWindowInterface *fw,
                             UicLanguage language,
                             QString *code,
                             QString *errorMessage);

    static bool showCodeDialog(const QDesignerFormWindowInterface *fw,
                               UicLanguage language,
                               QWidget *parent,
                               QString *errorMessage);

private slots:
    void slotSaveAs();
#if QT_CONFIG(clipboard)
    void copyAll();
#endif

private:
    void setCode(const QString &code);
    QString code() const;
    void setFormFileName(const QString &f);
    QString formFileName() const;
    void setMimeType(const QString &m);

    void warning(const QString &msg);

    struct CodeDialogPrivate;
    CodeDialogPrivate *m_impl;
};

} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // CODEPREVIEWDIALOG_H
