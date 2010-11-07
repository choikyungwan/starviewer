/***************************************************************************
 *   Copyright (C) 2005-2007 by Grup de Gràfics de Girona                  *
 *   http://iiia.udg.es/GGG/index.html?langu=uk                            *
 *                                                                         *
 *   Universitat de Girona                                                 *
 ***************************************************************************/
#ifndef UDG_STARVIEWER_APPLICATION
#define UDG_STARVIEWER_APPLICATION

#include <QDir>

namespace udg{

const QString StarviewerVersionString("0.9.2-RC3");
const QString StarviewerBuildID("2010110700");

//Indica per aquesta versió d'starviewer quina és la revisió de bd necessària
const int StarviewerDatabaseRevisionRequired(5114);

const QString OrganizationNameString("GILab");
const QString OrganizationDomainString("starviewer.udg.edu");

#ifdef STARVIEWER_LITE
    const QString ApplicationNameString("Starviewer Lite");
#else
    const QString ApplicationNameString("Starviewer");
#endif

const QString OrganizationEmailString("support@starviewer.udg.edu");
const QString OrganizationWebURL("http://starviewer.udg.edu");

// Paths per defecte on es guarden configuracions i arxius d'usuari
/// Path base per a dades d'usuari
const QString UserDataRootPath( QDir::homePath() + "/.starviewer/" );
/// Path del log
const QString UserLogsPath( UserDataRootPath + "log/" );
/// Ruta absoluta del fitxer de log
const QString UserLogsFile( UserLogsPath + "starviewer.log" );
/// Path dels hanging protocols d'usuari
const QString UserHangingProtocolsPath( UserDataRootPath + "hangingprotocols/" );

// TODO const QString LogConfigurationFileLocation();

}; // end namespace udg

#endif
