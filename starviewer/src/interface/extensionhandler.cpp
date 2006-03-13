/***************************************************************************
 *   Copyright (C) 2005 by Grup de Gr�fics de Girona                       *
 *   http://iiia.udg.es/GGG/index.html?langu=uk                            *
 *                                                                         *
 *   Universitat de Girona                                                 *
 ***************************************************************************/
#include "extensionhandler.h"

// qt
#include <QFileInfo>
#include <QDir>
// recursos
#include "volumerepository.h"
#include "input.h"
#include "output.h"
// aplicacions
#include "extensionworkspace.h"
#include "qapplicationmainwindow.h"

#include "extensionfactory.h"
#include "qmprextensioncreator.h"
#include "qmpr3dextensioncreator.h"
#include "qmpr3d2dextensioncreator.h"
#include "qtabaxisviewextensioncreator.h"

// Espai reservat pels include de les mini-apps
#include "appimportfile.h"
#include "qtabaxisview.h" 
#include "qmprextension.h"
#include "qmpr3dextension.h"
#include "qmpr3d2dextension.h"

// Fi de l'espai reservat pels include de les mini-apps

// PACS --------------------------------------------
#include "queryscreen.h"



namespace udg {

ExtensionHandler::ExtensionHandler( QApplicationMainWindow *mainApp , QObject *parent, const char *name)
 : QObject(parent )
{
    this->setObjectName( name );
    m_volumeRepository = udg::VolumeRepository::getRepository();
    m_inputReader = new udg::Input;
    m_outputWriter = new udg::Output;
    m_mainApp = mainApp;    
    // ::::::::::::::::
    // Inicialitzaci� de mini-aplicacions
    // ::::::::::::::::
    
    // Aqu� en principi nom�s farem l'inicialitzaci�
    m_importFileApp = 0;
    registerExtensions();
}

ExtensionHandler::~ExtensionHandler()
{
}

void ExtensionHandler::registerExtensions()
{
    // creem totes les inst�ncies dels creadors d'extensions
    m_qTabAxisViewExtensionCreator = new QTabAxisViewExtensionCreator(this);    
    m_qMPRExtensionCreator = new QMPRExtensionCreator( this );
    m_qMPR3DExtensionCreator = new QMPR3DExtensionCreator( this );
    m_qMPR3D2DExtensionCreator = new QMPR3D2DExtensionCreator( this );
    
    // al crear-se el handler inicialitzem el factory amb totes les aplicacions
    m_extensionFactory = new ExtensionFactory(this);
    m_extensionFactory->registerExtension( "Tab Axis View" , m_qTabAxisViewExtensionCreator );
    m_extensionFactory->registerExtension( "2D MPR Extension" , m_qMPRExtensionCreator );
    m_extensionFactory->registerExtension( "3D MPR Extension" , m_qMPR3DExtensionCreator );
    m_extensionFactory->registerExtension( "3D-2D MPR Extension" , m_qMPR3D2DExtensionCreator );
}


void ExtensionHandler::request( int who )
{

// \TODO: crear l'extensi� amb el factory ::createExtension, no com est� ara
//     QueryScreen *queryScreen = new QueryScreen;
    
    QTabAxisView *axisView = new QTabAxisView( m_mainApp );
    QMPRExtension *mprExtension = new QMPRExtension( 0 );
    QMPR3DExtension *mpr3DExtension = new QMPR3DExtension( 0 );
    QMPR3D2DExtension *mpr3D2DExtension = new QMPR3D2DExtension( 0 );
    /// \TODO la numeraci� �s completament temporal!!! s'haur� de canviar aquest sistema
    switch( who )
    {
    
    case 1:
        // open!
        // caldria comprovar si cal obrir una nova MainWindow
        if( m_volumeID.isNull() )
        {
            m_importFileApp = new AppImportFile();
            m_importFileApp->open();
            m_volumeID = m_importFileApp->getVolumeIdentifier();
            m_importFileApp->finish();
// **************************************************************************************
//
//    APLICACI� QUE S'OBRE PER DEFECTE QUAN OBRIM UN VOLUM
//
// **************************************************************************************
            request(2);
        }
        else
        {
            // ara com li diem que en la nova finestra volem que s'executi la petici� d'importar arxiu?
        }
    break;
    
    /// 2D MPR VIEW
    case 2:
        if( !m_volumeID.isNull() )
        {
// Aquest �s el "nou" MPR
            mprExtension->setInput( m_volumeRepository->getVolume( m_volumeID ) );
            m_mainApp->m_extensionWorkspace->addApplication( mprExtension , tr("2D MPR") );

        }
        else
        {
            // ara com li diem que en la nova finestra volem que s'executi la petici� d'importar arxiu?
        }
    break;
    
    /// MPR 3D VIEW
    case 3:
        if( !m_volumeID.isNull() )
        {
            mpr3DExtension->setInput( m_volumeRepository->getVolume( m_volumeID ) );
            m_mainApp->m_extensionWorkspace->addApplication( mpr3DExtension , tr("3D MPR") );
        }
        else
        {
            // ara com li diem que en la nova finestra volem que s'executi la petici� d'importar arxiu?
        }
    break;
    
    /// MPR 3D-2D VIEW
    case 4:
        if( !m_volumeID.isNull() )
        {
            mpr3D2DExtension->setInput( m_volumeRepository->getVolume( m_volumeID ) );
            m_mainApp->m_extensionWorkspace->addApplication( mpr3D2DExtension , tr("3D-2D MPR") );
        }
        else
        {
            // ara com li diem que en la nova finestra volem que s'executi la petici� d'importar arxiu?
        }
    break;
    
    default:
        axisView->setInput( m_volumeRepository->getVolume( m_volumeID ) );
        m_mainApp->m_extensionWorkspace->addApplication( axisView , tr("Volume Axis View"));
    break;
    }
}

void ExtensionHandler::request( const QString &who )
{
}

void ExtensionHandler::introduceApplications()
{
}

bool ExtensionHandler::open( QString fileName )
{
    bool ok = true; 
    
    if ( ! m_volumeID.isNull() )
    {
        //Si ja tenim obert un model, obrim una finestra nova ???
    }
    else
    {
        // indiquem que ens obri el fitxer
        if( QFileInfo( fileName ).suffix() == "dcm") // petita prova per provar lectura de DICOM's
        {
            if( m_inputReader->readSeries( QFileInfo(fileName).dir().absolutePath().toLatin1() ) )
            { 
                // creem el volum
                udg::Volume *dummyVolume = m_inputReader->getData();
                // afegim el nou volum al repositori
                m_volumeID = m_volumeRepository->addVolume( dummyVolume );            
                
                request(5);
            }
            else
            {
                // no s'ha pogut obrir l'arxiu per algun motiu
                ok = false;
            }
        }
        else
        {
            if( m_inputReader->openFile( fileName.toLatin1() ) )
            { 
                // creem el volum
                udg::Volume *dummyVolume = m_inputReader->getData();
                // afegim el nou volum al repositori
                m_volumeID = m_volumeRepository->addVolume( dummyVolume );            
                request(5);
            }
            else
            {
                // no s'ha pogut obrir l'arxiu per algun motiu
                ok = false;
            }
        }
    } 
    return ok;  
}

};  // end namespace udg 
