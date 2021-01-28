/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/


#include "qdesigner_formeditorcommand_p.h"
#include <QtDesigner/abstractformeditor.h>

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

// ---- QDesignerFormEditorCommand ----
QDesignerFormEditorCommand::QDesignerFormEditorCommand(const QString &description, QDesignerFormEditorInterface *core)
    : QUndoCommand(description),
      m_core(core)
{
}

QDesignerFormEditorInterface *QDesignerFormEditorCommand::core() const
{
    return m_core;
}

} // namespace qdesigner_internal

QT_END_NAMESPACE
