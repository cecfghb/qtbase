/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/private/qxunittestlogger_p.h>
#include <QtTest/private/qtestelement_p.h>
#include <QtTest/private/qtestxunitstreamer_p.h>
#include <QtTest/qtestcase.h>
#include <QtTest/private/qtestresult_p.h>
#include <QtTest/private/qbenchmark_p.h>

#include <string.h>

QT_BEGIN_NAMESPACE

QXunitTestLogger::QXunitTestLogger(const char *filename)
    : QAbstractTestLogger(filename)
    , listOfTestcases(0)
    , currentLogElement(0)
    , errorLogElement(0)
    , logFormatter(0)
    , testCounter(0)
    , failureCounter(0)
    , errorCounter(0)
{
}

QXunitTestLogger::~QXunitTestLogger()
{
    delete currentLogElement;
    delete logFormatter;
}

void QXunitTestLogger::startLogging()
{
    QAbstractTestLogger::startLogging();

    logFormatter = new QTestXunitStreamer(this);
    delete errorLogElement;
    errorLogElement = new QTestElement(QTest::LET_SystemError);
}

void QXunitTestLogger::stopLogging()
{
    QTestElement *iterator = listOfTestcases;

    char buf[10];

    currentLogElement = new QTestElement(QTest::LET_TestSuite);
    currentLogElement->addAttribute(QTest::AI_Name, QTestResult::currentTestObjectName());

    qsnprintf(buf, sizeof(buf), "%i", testCounter);
    currentLogElement->addAttribute(QTest::AI_Tests, buf);

    qsnprintf(buf, sizeof(buf), "%i", failureCounter);
    currentLogElement->addAttribute(QTest::AI_Failures, buf);

    qsnprintf(buf, sizeof(buf), "%i", errorCounter);
    currentLogElement->addAttribute(QTest::AI_Errors, buf);

    QTestElement *property;
    QTestElement *properties = new QTestElement(QTest::LET_Properties);

    property = new QTestElement(QTest::LET_Property);
    property->addAttribute(QTest::AI_Name, "QTestVersion");
    property->addAttribute(QTest::AI_PropertyValue, QTEST_VERSION_STR);
    properties->addLogElement(property);

    property = new QTestElement(QTest::LET_Property);
    property->addAttribute(QTest::AI_Name, "QtVersion");
    property->addAttribute(QTest::AI_PropertyValue, qVersion());
    properties->addLogElement(property);

    currentLogElement->addLogElement(properties);

    currentLogElement->addLogElement(iterator);

    /* For correct indenting, make sure every testcase knows its parent */
    QTestElement* testcase = iterator;
    while (testcase) {
        testcase->setParent(currentLogElement);
        testcase = testcase->nextElement();
    }

    currentLogElement->addLogElement(errorLogElement);

    QTestElement *it = currentLogElement;
    logFormatter->output(it);

    QAbstractTestLogger::stopLogging();
}

void QXunitTestLogger::enterTestFunction(const char *function)
{
    currentLogElement = new QTestElement(QTest::LET_TestCase);
    currentLogElement->addAttribute(QTest::AI_Name, function);
    currentLogElement->addToList(&listOfTestcases);

    ++testCounter;
}

void QXunitTestLogger::leaveTestFunction()
{
}

void QXunitTestLogger::addIncident(IncidentTypes type, const char *description,
                                   const char *file, int line)
{
    const char *typeBuf = 0;
    char buf[100];

    switch (type) {
    case QAbstractTestLogger::XPass:
        ++failureCounter;
        typeBuf = "xpass";
        break;
    case QAbstractTestLogger::Pass:
        typeBuf = "pass";
        break;
    case QAbstractTestLogger::XFail:
        typeBuf = "xfail";
        break;
    case QAbstractTestLogger::Fail:
        ++failureCounter;
        typeBuf = "fail";
        break;
    default:
        typeBuf = "??????";
        break;
    }

    if (type == QAbstractTestLogger::Fail || type == QAbstractTestLogger::XPass) {
        QTestElement *failureElement = new QTestElement(QTest::LET_Failure);
        failureElement->addAttribute(QTest::AI_Result, typeBuf);
        if (file)
            failureElement->addAttribute(QTest::AI_File, file);
        else
            failureElement->addAttribute(QTest::AI_File, "");
        qsnprintf(buf, sizeof(buf), "%i", line);
        failureElement->addAttribute(QTest::AI_Line, buf);
        failureElement->addAttribute(QTest::AI_Description, description);
        addTag(failureElement);
        currentLogElement->addLogElement(failureElement);
    }

    /*
        Only one result can be shown for the whole testfunction.
        Check if we currently have a result, and if so, overwrite it
        iff the new result is worse.
    */
    QTestElementAttribute* resultAttr =
        const_cast<QTestElementAttribute*>(currentLogElement->attribute(QTest::AI_Result));
    if (resultAttr) {
        const char* oldResult = resultAttr->value();
        bool overwrite = false;
        if (!strcmp(oldResult, "pass")) {
            overwrite = true;
        }
        else if (!strcmp(oldResult, "xfail")) {
            overwrite = (type == QAbstractTestLogger::XPass || type == QAbstractTestLogger::Fail);
        }
        else if (!strcmp(oldResult, "xpass")) {
            overwrite = (type == QAbstractTestLogger::Fail);
        }
        if (overwrite) {
            resultAttr->setPair(QTest::AI_Result, typeBuf);
        }
    }
    else {
        currentLogElement->addAttribute(QTest::AI_Result, typeBuf);
    }

    if (file)
        currentLogElement->addAttribute(QTest::AI_File, file);
    else
        currentLogElement->addAttribute(QTest::AI_File, "");

    qsnprintf(buf, sizeof(buf), "%i", line);
    currentLogElement->addAttribute(QTest::AI_Line, buf);

    /*
        Since XFAIL does not add a failure to the testlog in xunitxml, add a message, so we still
        have some information about the expected failure.
    */
    if (type == QAbstractTestLogger::XFail) {
        QXunitTestLogger::addMessage(QAbstractTestLogger::Info, description, file, line);
    }
}

void QXunitTestLogger::addBenchmarkResult(const QBenchmarkResult &result)
{
    QTestElement *benchmarkElement = new QTestElement(QTest::LET_Benchmark);

    benchmarkElement->addAttribute(
        QTest::AI_Metric,
        QTest::benchmarkMetricName(QBenchmarkTestMethodData::current->result.metric));
    benchmarkElement->addAttribute(QTest::AI_Tag, result.context.tag.toUtf8().data());
    benchmarkElement->addAttribute(QTest::AI_Value, QByteArray::number(result.value).constData());

    char buf[100];
    qsnprintf(buf, sizeof(buf), "%i", result.iterations);
    benchmarkElement->addAttribute(QTest::AI_Iterations, buf);
    currentLogElement->addLogElement(benchmarkElement);
}

void QXunitTestLogger::addTag(QTestElement* element)
{
    const char *tag = QTestResult::currentDataTag();
    const char *gtag = QTestResult::currentGlobalDataTag();
    const char *filler = (tag && gtag) ? ":" : "";
    if ((!tag || !tag[0]) && (!gtag || !gtag[0])) {
        return;
    }

    if (!tag) {
        tag = "";
    }
    if (!gtag) {
        gtag = "";
    }

    QTestCharBuffer buf;
    QTest::qt_asprintf(&buf, "%s%s%s", gtag, filler, tag);
    element->addAttribute(QTest::AI_Tag, buf.constData());
}

void QXunitTestLogger::addMessage(MessageTypes type, const char *message, const char *file, int line)
{
    QTestElement *errorElement = new QTestElement(QTest::LET_Error);
    const char *typeBuf = 0;

    switch (type) {
    case QAbstractTestLogger::Warn:
        typeBuf = "warn";
        break;
    case QAbstractTestLogger::QSystem:
        typeBuf = "system";
        break;
    case QAbstractTestLogger::QDebug:
        typeBuf = "qdebug";
        break;
    case QAbstractTestLogger::QWarning:
        typeBuf = "qwarn";
        break;
    case QAbstractTestLogger::QFatal:
        typeBuf = "qfatal";
        break;
    case QAbstractTestLogger::Skip:
        typeBuf = "skip";
        break;
    case QAbstractTestLogger::Info:
        typeBuf = "info";
        break;
    default:
        typeBuf = "??????";
        break;
    }

    errorElement->addAttribute(QTest::AI_Type, typeBuf);
    errorElement->addAttribute(QTest::AI_Description, message);
    addTag(errorElement);

    if (file)
        errorElement->addAttribute(QTest::AI_File, file);
    else
        errorElement->addAttribute(QTest::AI_File, "");

    char buf[100];
    qsnprintf(buf, sizeof(buf), "%i", line);
    errorElement->addAttribute(QTest::AI_Line, buf);

    currentLogElement->addLogElement(errorElement);
    ++errorCounter;

    // Also add the message to the system error log (i.e. stderr), if one exists
    if (errorLogElement) {
        QTestElement *systemErrorElement = new QTestElement(QTest::LET_Error);
        systemErrorElement->addAttribute(QTest::AI_Description, message);
        errorLogElement->addLogElement(systemErrorElement);
    }
}

QT_END_NAMESPACE

