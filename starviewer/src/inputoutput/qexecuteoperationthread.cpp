/***************************************************************************
 *   Copyright (C) 2005 by Grup de Gr�fics de Girona                       *
 *   http://iiia.udg.es/GGG/index.html?langu=uk                            *
 *                                                                         *
 *   Universitat de Girona                                                 *
 ***************************************************************************/
#include "qexecuteoperationthread.h"
#include "studymask.h"
#include "pacsserver.h"
#include "retrieveimages.h"
#include "operation.h"
#include "queueoperationlist.h"
#include "processimagesingleton.h"
#include "starviewerprocessimage.h"
#include "cachepacs.h"
#include "harddiskinformation.h"
#include "scalestudy.h"
#include <QSemaphore>
#include "cachepool.h"
#include "starviewersettings.h"
#include "const.h"
#include "cachelayer.h"


namespace udg {

QSemaphore m_semaphor(1);//controlar l'acces a la variable m_stop

//constructor
QExecuteOperationThread::QExecuteOperationThread(QObject *parent)
 : QThread(parent)
{
     m_stop = true;
}

void QExecuteOperationThread::queueOperation(Operation operation)
{
    QueueOperationList *queueOperationList = QueueOperationList::getQueueOperationList();
    
    
    m_semaphor.acquire();
    
    //la variable m_stop controla si el thread est� engegat o parat!
    queueOperationList->insertOperation(operation); 
    if(m_stop = true)
    {   //si parat l'engeguem
        m_stop = false;
        start();
    }
    
    m_semaphor.release();
}

//s'executa des del thread
void QExecuteOperationThread::run()
{
    QueueOperationList *queueOperationList = QueueOperationList::getQueueOperationList();
    
    //creem les connexions amb l'objecte QRetrieveScreen, per indicar descarregues
    
    while (!m_stop)
    {
        Operation operation;
        
        operation = queueOperationList->getMaximumPriorityOperation();
        
        switch (operation.getOperation())
        {
            case operationRetrieve : 
                 m_view = false;
                 retrieveStudy(operation,m_view);
                 break;
            case operationView : 
                 m_view = true;
                 retrieveStudy(operation,m_view);
                 break;
        }
        
        //comprovem si hem de parar
        m_semaphor.acquire();
        m_stop = queueOperationList->isEmpty();
        m_semaphor.release();
    } 
}

//descarrega un estudi
void QExecuteOperationThread::retrieveStudy(Operation operation,bool view)
{
    StarviewerProcessImage *sProcessImg = new StarviewerProcessImage::StarviewerProcessImage();
    QString studyUID;
    Status state,retState;

    ProcessImageSingleton *piSingleton = ProcessImageSingleton::getProcessImageSingleton();
    CachePacs *localCache =  CachePacs::getCachePacs();
    ScaleStudy scaleStudy;
    RetrieveImages retrieve;
    bool enoughSpace;
    
    studyUID = operation.getStudyMask().getStudyUID().c_str();
        
    state = enoughFreeSpace( enoughSpace );    
        
    //s'indica que comen�a la descarreca de l'estudi al qretrieveScreen
    emit( setStudyRetrieving( studyUID.toAscii().constData() ) );
   
   
    if ( !state.good() || !enoughSpace ) 
    {
        emit( setErrorRetrieving( studyUID.toAscii().constData() ) );
        localCache->delStudy( studyUID.toAscii().constData());
        return;
    }
    
    PacsServer pacsConnection(operation.getPacsParameters());//connemtem al pacs
    state = pacsConnection.Connect(PacsServer::retrieveImages,PacsServer::studyLevel);    
    if (!state.good())
    {   
        emit( setErrorRetrieving( studyUID.toAscii().constData() ) );
        localCache->delStudy( studyUID.toAscii().constData()) ;        
        return; 
    }
    
    //passem els parametres a la classe retrieveImages
    retrieve.setConnection( pacsConnection.getConnection() );  
    retrieve.setMask( operation.getStudyMask() ); 
    retrieve.setNetwork( pacsConnection.getNetwork() );
    
    //afegim a la ProcssesImageSingletton quin objecte s'encarregarr� de processar les imatges descarregades
    piSingleton->addNewProcessImage( studyUID.toAscii().constData(),sProcessImg) ;
   
    //connectem els signals del starviewerProcessImage
    connect( sProcessImg , SIGNAL( seriesView( QString ) ) , this , SLOT( firstSeriesRetrieved( QString ) ) );
    connect( sProcessImg , SIGNAL( imageRetrieved( QString , int ) ) , this , SLOT( imageRetrievedSlot( QString , int ) ) );
    connect( sProcessImg , SIGNAL( seriesRetrieved( QString ) ) , this , SLOT( seriesRetrievedSlot( QString ) ) );

    retState = retrieve.moveSCU();
    if (!retState.good() || sProcessImg->getErrorRetrieving() )
    {//si s'ha produit algun error ho indiquem i esborrem l'estudi 
         emit( setErrorRetrieving( studyUID.toAscii().constData() ) );
        localCache->delStudy( studyUID.toAscii().constData() );
    }
    else 
    {    
        scaleStudy.scale( studyUID.toAscii().constData() ); //escalem l'estudi per la previsualitzaci� de la cach�  
        emit( setStudyRetrieved( studyUID.toAscii().constData() ) );// descarregat
        localCache->setStudyRetrieved( studyUID.toAscii().constData() ); //posem l'estudi com a descarregat
    }
    
    pacsConnection.Disconnect();

    //esborrem el processImage de la llista de processImage encarregat de processar la informaci� per cada imatge descarregada
    piSingleton->delProcessImage( studyUID.toAscii().constData() );
    
}

void QExecuteOperationThread::firstSeriesRetrieved( QString studyUID )
{
     cout<<"entro\n";
     if ( m_view ) emit( viewStudy( studyUID ) ); //signal cap a QueryScreen
}

void QExecuteOperationThread::imageRetrievedSlot( QString studyUID , int imageNumber)
{
    emit( imageRetrieved( studyUID , imageNumber ) ) ;
}

void QExecuteOperationThread::seriesRetrievedSlot( QString studyUID)
{
    emit( seriesRetrieved( studyUID ));
}

Status QExecuteOperationThread::enoughFreeSpace( bool &enoughSpace)
{
    HardDiskInformation hardDiskInformation;
    CachePool pool;
    StarviewerSettings settings;
    unsigned int freePoolSpace, freeSystemSpace;
    Status state;
    CacheLayer cacheLayer;
    
    freeSystemSpace = hardDiskInformation.getNumberOfFreeMBytes( settings.getCacheImagePath() );
    if ( freeSystemSpace == 0 ) return state.setStatus( ERROR );
    pool.getPoolFreeSpace( freePoolSpace );  
    
    //si no hi ha suficient espai lliure a la cache o al disc dur intera esborrar dos Gb
    if ( freeSystemSpace <= CachePool::MinimumMBytesOfDiskSpaceRequired || 
         freePoolSpace <= CachePool::MinimumMBytesOfDiskSpaceRequired )
    {
        cacheLayer.deleteOldStudies( CachePool::MBytesToEraseWhenDiskOrCacheFull ); //esborrem els estudis 
        
        freeSystemSpace = hardDiskInformation.getNumberOfFreeMBytes( settings.getCacheImagePath() );
        if ( freeSystemSpace == 0 ) return state.setStatus( ERROR );
        
        pool.getPoolFreeSpace( freePoolSpace );  
        
        //hem intentat esborrar pero no hi ha hagut suficient espai
        if ( freeSystemSpace <=  CachePool::MinimumMBytesOfDiskSpaceRequired || 
             freePoolSpace <=  CachePool::MinimumMBytesOfDiskSpaceRequired )
        {
            //si no hi ha suficient espai,significa que no hi ha prou espai al disc, perque de la cache sempre podem alliberar espai
            enoughSpace = false;
        }
        else enoughSpace = true;
    }
    else enoughSpace = true;
    
    return state.setStatus( CORRECT );
}

QExecuteOperationThread::~QExecuteOperationThread()
{
}


}
