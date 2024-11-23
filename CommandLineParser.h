#ifndef COMMANDLINEPARSER_H
#define COMMANDLINEPARSER_H


#include <QCoreApplication>
#include <QCommandLineParser>


class CommandLineParser : public QCommandLineParser
{
private:
    bool        m_HelpSet      = false;
    bool        m_VersionSet   = false;
    bool        m_SysConfSet   = false;
    bool        m_TestFileSet  = false;
    QStringList m_unknownOptionNames;

public:
    CommandLineParser();

    void        process(const QStringList &arguments);
    QStringList unknownOptionNames()    const;
    bool        isHelpSet()             const;
    bool        isVersionSet()          const;
    bool        isSysConfSet()          const;
    bool        isTestFileSet()         const;
};

#endif // COMMANDLINEPARSER_H
