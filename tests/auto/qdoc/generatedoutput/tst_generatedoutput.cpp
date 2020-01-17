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
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest>

class tst_generatedOutput : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    // HTML generator
    void htmlFromQDocFile();
    void htmlFromCpp();
    void htmlFromQml();
    void htmlFromCppBug80259();

    // WebXML generator
    void webXmlFromQDocFile();
    void webXmlFromCpp();
    void webXmlFromQml();
    void webXmlFromCppBug80259();

    // DocBook generator
    void docBookFromQDocFile();
    void docBookFromCpp();
    void docBookFromQml();

    // Output format independent tests
    void examplesManifestXml();
    void ignoresinceVariable();
    void templateParameters();

private:
    QScopedPointer<QTemporaryDir> m_outputDir;
    QString m_qdoc;

    void runQDocProcess(const QStringList &arguments);
    void compareLineByLine(const QStringList &expectedFiles);
    void testAndCompare(const char *input, const char *outNames, const char *extraParams = nullptr,
                        const char *outputPathPrefix = nullptr);
};

void tst_generatedOutput::initTestCase()
{
    // Build the path to the QDoc binary the same way moc tests do for moc.
    const auto binpath = QLibraryInfo::location(QLibraryInfo::BinariesPath);
    const auto extension = QSysInfo::productType() == "windows" ? ".exe" : "";
    m_qdoc = binpath + QLatin1String("/qdoc") + extension;
}

void tst_generatedOutput::init()
{
    m_outputDir.reset(new QTemporaryDir());
    if (!m_outputDir->isValid()) {
        const QString errorMessage =
                "Couldn't create temporary directory: " + m_outputDir->errorString();
        QFAIL(qPrintable(errorMessage));
    }
}

void tst_generatedOutput::runQDocProcess(const QStringList &arguments)
{
    QProcess qdocProcess;
    qdocProcess.setProgram(m_qdoc);
    qdocProcess.setArguments(arguments);
    qdocProcess.start();
    qdocProcess.waitForFinished();

    if (qdocProcess.exitCode() == 0)
        return;

    QString output = qdocProcess.readAllStandardOutput();
    QString errors = qdocProcess.readAllStandardError();

    qInfo() << "QDoc exited with exit code" << qdocProcess.exitCode();
    if (output.size() > 0)
        qInfo() << "Received output:\n" << output;
    if (errors.size() > 0)
        qInfo() << "Received errors:\n" << errors;

    QFAIL("Running QDoc failed. See output above.");
}

void tst_generatedOutput::compareLineByLine(const QStringList &expectedFiles)
{
    for (const auto &file : expectedFiles) {
        QString expected(QFINDTESTDATA("/expected_output/" + file));
        QString actual(m_outputDir->path() + "/" + file);

        QFile expectedFile(expected);
        if (!expectedFile.open(QIODevice::ReadOnly))
            QFAIL("Cannot open expected data file!");
        QTextStream expectedIn(&expectedFile);

        QFile actualFile(actual);
        if (!actualFile.open(QIODevice::ReadOnly))
            QFAIL("Cannot open actual data file!");
        QTextStream actualIn(&actualFile);

        const QLatin1String delim(": ");
        int lineNumber = 0;
        while (!expectedIn.atEnd() && !actualIn.atEnd()) {
            lineNumber++;
            QString prefix = file + delim + QString::number(lineNumber) + delim;
            QString expectedLine = prefix + expectedIn.readLine();
            QString actualLine = prefix + actualIn.readLine();
            QCOMPARE(actualLine, expectedLine);
        }
    }
}

void tst_generatedOutput::testAndCompare(const char *input, const char *outNames,
                                         const char *extraParams, const char *outputPathPrefix)
{
    QStringList args { "-outputdir", m_outputDir->path() + "/" + outputPathPrefix,
                       QFINDTESTDATA(input) };
    if (extraParams)
        args << QString(QLatin1String(extraParams)).split(QChar(' '));

    runQDocProcess(args);

    if (QTest::currentTestFailed())
        return;

    QStringList expectedOuts(QString(QLatin1String(outNames)).split(QChar(' ')));
    if (outputPathPrefix)
        for (auto &expectedOut : expectedOuts)
            expectedOut = QString(outputPathPrefix) + "/" + expectedOut;

    compareLineByLine(expectedOuts);
}

void tst_generatedOutput::htmlFromQDocFile()
{
    testAndCompare("test.qdocconf",
                   "qdoctests-qdocfileoutput.html "
                   "qdoctests-qdocfileoutput-linking.html");
}

void tst_generatedOutput::htmlFromCpp()
{
    testAndCompare("testcpp.qdocconf",
                   "testcpp-module.html "
                   "testqdoc-test.html "
                   "testqdoc-test-members.html "
                   "testqdoc.html");
}

void tst_generatedOutput::htmlFromQml()
{
    testAndCompare("testqml.qdocconf",
                   "test-componentset-example.html "
                   "uicomponents-qmlmodule.html "
                   "qdoc-test-qmlmodule.html "
                   "qml-qdoc-test-abstractparent.html "
                   "qml-qdoc-test-child.html "
                   "qml-qdoc-test-doctest.html "
                   "qml-qdoc-test-type-members.html "
                   "qml-qdoc-test-type.html "
                   "qml-uicomponents-progressbar.html "
                   "qml-uicomponents-switch.html "
                   "qml-uicomponents-tabwidget.html "
                   "qml-int.html");
}

void tst_generatedOutput::htmlFromCppBug80259()
{
    testAndCompare("bug80259/testmodule.qdocconf",
                   "first.html "
                   "second.html "
                   "third.html "
                   "index.html");
}

void tst_generatedOutput::webXmlFromQDocFile()
{
    testAndCompare("webxml_test.qdocconf",
                   "html/qdoctests-qdocfileoutput.webxml "
                   "html/qdoctests-qdocfileoutput-linking.webxml");
}

void tst_generatedOutput::webXmlFromCpp()
{
    testAndCompare("webxml_testcpp.qdocconf",
                   "html/testcpp-module.webxml "
                   "html/testqdoc-test.webxml "
                   "html/testqdoc-testderived.webxml");
}

void tst_generatedOutput::webXmlFromQml()
{
    testAndCompare("webxml_testqml.qdocconf",
                   "html/test-componentset-example.webxml "
                   "html/uicomponents-qmlmodule.webxml");
}

void tst_generatedOutput::webXmlFromCppBug80259()
{
    testAndCompare("bug80259/webxml_testmodule.qdocconf",
                   "html/first.webxml "
                   "html/second.webxml "
                   "html/third.webxml "
                   "html/index.webxml");
}

void tst_generatedOutput::docBookFromQDocFile()
{
    testAndCompare("docbook_test.qdocconf",
                   "docbook/qdoctests-qdocfileoutput.xml "
                   "docbook/qdoctests-qdocfileoutput-linking.xml");
}

void tst_generatedOutput::docBookFromCpp()
{
    testAndCompare("docbook_testcpp.qdocconf",
                   "docbook/testcpp-module.xml "
                   "docbook/testqdoc-test.xml "
                   "docbook/testqdoc-testderived.xml "
                   "docbook/testqdoc.xml");
}

void tst_generatedOutput::docBookFromQml()
{
    testAndCompare("docbook_testqml.qdocconf",
                   "docbook/test-componentset-example.xml "
                   "docbook/uicomponents-qmlmodule.xml "
                   "docbook/qdoc-test-qmlmodule.xml "
                   "docbook/qml-qdoc-test-abstractparent.xml "
                   "docbook/qml-qdoc-test-child.xml "
                   "docbook/qml-qdoc-test-doctest.xml "
                   "docbook/qml-qdoc-test-type.xml "
                   "docbook/qml-uicomponents-progressbar.xml "
                   "docbook/qml-uicomponents-switch.xml "
                   "docbook/qml-uicomponents-tabwidget.xml "
                   "docbook/qml-int.xml");
}

void tst_generatedOutput::examplesManifestXml()
{
    testAndCompare("examples-qhp.qdocconf", "examples-manifest.xml");
}

void tst_generatedOutput::ignoresinceVariable()
{
    testAndCompare("ignoresince.qdocconf",
                   "ignoresince/testqdoc.html "
                   "ignoresince/testqdoc-test.html");
}

void tst_generatedOutput::templateParameters()
{
  testAndCompare("testtemplate.qdocconf", "template/testqdoc-test.html "
                                          "template/foo.html "
                                          "template/bar.html "
                                          "template/baz.html");
}

QTEST_APPLESS_MAIN(tst_generatedOutput)

#include "tst_generatedoutput.moc"
