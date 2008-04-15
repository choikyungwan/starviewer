/***************************************************************************
 *   Copyright (C) 2005-2007 by Grup de Gràfics de Girona                  *
 *   http://iiia.udg.es/GGG/index.html?langu=uk                            *
 *                                                                         *
 *   Universitat de Girona                                                 *
 ***************************************************************************/
#include "referencelinestool.h"
#include "referencelinestooldata.h"
#include "q2dviewer.h"
#include "logging.h"
#include "volume.h"
#include "series.h"
#include "imageplane.h"
#include "drawer.h"
#include "drawerpolygon.h"
#include "drawerline.h"
#include "mathtools.h"
// vtk
#include <vtkPlane.h>

namespace udg {

ReferenceLinesTool::ReferenceLinesTool( QViewer *viewer, QObject *parent )
 : Tool(viewer, parent), m_projectedReferencePlane(0), m_lowerProjectedIntersection(0), m_backgroundLowerProjectedIntersection(0), m_upperProjectedIntersection(0), m_backgroundUpperProjectedIntersection(0)
{
    m_toolName = "ReferenceLinesTool";
    m_hasSharedData = true;

    m_myData = new ReferenceLinesToolData;
    m_toolData = m_myData;
    connect( m_toolData, SIGNAL(changed()), SLOT(updateProjectionLines()) );

    m_2DViewer = qobject_cast<Q2DViewer *>( viewer );
    if( !m_2DViewer )
    {
        DEBUG_LOG(QString("El casting no ha funcionat!!! És possible que viewer no sigui un Q2DViewer!!!-> ") + viewer->metaObject()->className() );
    }

    createPrimitives();
    refreshReferenceViewerData();

    // cada cop que el viewer canvïi d'input, hem d'actualitzar el frame of reference
    connect( m_2DViewer, SIGNAL(volumeChanged(Volume *) ), SLOT( refreshReferenceViewerData() ) );
    // cada cop que el viewer canvïi de llesca, hem d'actualitzar el pla de projecció
    connect( m_2DViewer, SIGNAL(sliceChanged(int)), SLOT(updateImagePlane()) );

    connect( m_2DViewer, SIGNAL(selected()),SLOT(refreshReferenceViewerData()) );
}

ReferenceLinesTool::~ReferenceLinesTool()
{
    // ja no som propietaris de les línies creades
    m_projectedReferencePlane->decreaseReferenceCount();
    m_upperProjectedIntersection->decreaseReferenceCount();
    m_backgroundUpperProjectedIntersection->decreaseReferenceCount();
    m_lowerProjectedIntersection->decreaseReferenceCount();
    m_backgroundLowerProjectedIntersection->decreaseReferenceCount();

    // ara al fer delete, s'esborraran automàticament del drawer, ja que nosaltres érem els únics propietaris
    delete m_projectedReferencePlane;
    delete m_upperProjectedIntersection;
    delete m_backgroundUpperProjectedIntersection;
    delete m_lowerProjectedIntersection;
    delete m_backgroundLowerProjectedIntersection;
}

void ReferenceLinesTool::setToolData(ToolData * data)
{
    m_toolData = data;
    m_myData = qobject_cast<ReferenceLinesToolData *>(data);
    // quan canvïn les dades (ImagePlane), actualitzem les línies de projecció
    connect( m_toolData, SIGNAL(changed()), SLOT(updateProjectionLines()) );
}

void ReferenceLinesTool::updateProjectionLines()
{
    // en cas que no sigui el viewer que estem modificant
    if( !m_2DViewer->isActive() )
    {
        // intentarem projectar el pla que hi ha a m_myData
        // primer cal que comparteixin el mateix FrameOfReference
        if( m_myFrameOfReferenceUID == m_myData->getFrameOfReferenceUID() )
        {
            // aquí ja ho deixem en mans de la projecció
            projectIntersection( m_myData->getImagePlane(), m_2DViewer->getCurrentImagePlane() );
        }
    }
}

void ReferenceLinesTool::createPrimitives()
{
    m_projectedReferencePlane = new DrawerPolygon;
    // TODO sucedani d'smart pointer(TM)
    m_projectedReferencePlane->increaseReferenceCount();
// descomentar aquestes 2 linies si es vol mostrar el poligon del pla projectat
//     m_2DViewer->getDrawer()->draw( m_projectedReferencePlane, QViewer::Top2DPlane );
//     m_2DViewer->getDrawer()->addToGroup( m_projectedReferencePlane, "ReferenceLines" );

    //
    // linia superior projectada de tall entre els plans localitzador i referencia
    //

    // linia de background, igual a la següent, per fer-la ressaltar a la imatge TODO es podria estalviar aquesta linia de mes si el propi drawer
    // admetes "background" en una linia amb "stiple" discontinu

    m_backgroundUpperProjectedIntersection = new DrawerLine;
    // TODO sucedani d'smart pointer(TM)
    m_backgroundUpperProjectedIntersection->increaseReferenceCount();

    m_backgroundUpperProjectedIntersection->setColor( QColor(0,0,0) );
    m_2DViewer->getDrawer()->draw( m_backgroundUpperProjectedIntersection, QViewer::Top2DPlane );
    m_2DViewer->getDrawer()->addToGroup( m_backgroundUpperProjectedIntersection, "ReferenceLines" );


    // linia "de punts"
    m_upperProjectedIntersection = new DrawerLine;
    // TODO sucedani d'smart pointer(TM)
    m_upperProjectedIntersection->increaseReferenceCount();

    m_upperProjectedIntersection->setColor( QColor(255,160,0) );
    m_upperProjectedIntersection->setLinePattern( DrawerPrimitive::DiscontinuousLinePattern );
    m_2DViewer->getDrawer()->draw( m_upperProjectedIntersection, QViewer::Top2DPlane );
    m_2DViewer->getDrawer()->addToGroup( m_upperProjectedIntersection, "ReferenceLines" );

    //
    // linia inferior projectada de tall entre els plans localitzador i referencia
    //

    // línia de background
    m_backgroundLowerProjectedIntersection = new DrawerLine;
    // TODO sucedani d'smart pointer(TM)
    m_backgroundLowerProjectedIntersection->increaseReferenceCount();

    m_backgroundLowerProjectedIntersection->setColor( QColor(0,0,0) );
    m_2DViewer->getDrawer()->draw( m_backgroundLowerProjectedIntersection, QViewer::Top2DPlane );
    m_2DViewer->getDrawer()->addToGroup( m_backgroundLowerProjectedIntersection, "ReferenceLines" );

    m_lowerProjectedIntersection = new DrawerLine;
    // TODO sucedani d'smart pointer(TM)
    m_lowerProjectedIntersection->increaseReferenceCount();

    m_lowerProjectedIntersection->setColor( QColor(255,160,0) );
    m_lowerProjectedIntersection->setLinePattern( DrawerPrimitive::DiscontinuousLinePattern );
    m_2DViewer->getDrawer()->draw( m_lowerProjectedIntersection, QViewer::Top2DPlane );
    m_2DViewer->getDrawer()->addToGroup( m_lowerProjectedIntersection, "ReferenceLines" );

    // TODO mirar si ens podem estalviar de cridar aquest metode aqui
    m_2DViewer->getDrawer()->showGroup("ReferenceLines");
}

void ReferenceLinesTool::projectIntersection(ImagePlane *referencePlane, ImagePlane *localizerPlane)
{
    if( !(referencePlane && localizerPlane) )
        return;

    // primer mirem que siguin plans diferents
    if( *localizerPlane != *referencePlane )
    {
        //
        // projecció de la intersecció dels plans
        //
        /// llegir http://fixunix.com/dicom/51195-scanogram-lines-mr.html

        // recollim les dades del pla del localitzador sobre el qual volem projectar el de referència
        double localizerNormalVector[3], localizerOrigin[3];
        localizerPlane->getNormalVector( localizerNormalVector );
        localizerPlane->getOrigin( localizerOrigin );

        // calculem totes les possibles interseccions
        QList< QVector<double> > upperPlaneBounds = referencePlane->getUpperBounds();
        double firstIntersectionPoint[3], secondIntersectionPoint[3];

        int numberOfIntersections = this->getIntersections( upperPlaneBounds.at(0), upperPlaneBounds.at(1), upperPlaneBounds.at(2), upperPlaneBounds.at(3), localizerPlane, firstIntersectionPoint, secondIntersectionPoint );

        //
        // TODO mirar exactament quan cal amagar les línies i quan no, depenent de les interseccions trobades
        //
        // un cop tenim les interseccions nomes cal projectar-les i pintar la linia
        DEBUG_LOG(" ======== Nombre d'interseccions entre plans: " +  QString::number( numberOfIntersections ) );
        if( numberOfIntersections == 2 )
        {
            m_2DViewer->projectDICOMPointToCurrentDisplayedImage( firstIntersectionPoint, firstIntersectionPoint );
            m_2DViewer->projectDICOMPointToCurrentDisplayedImage( secondIntersectionPoint, secondIntersectionPoint );

            // linia discontinua
            m_upperProjectedIntersection->setFirstPoint( firstIntersectionPoint );
            m_upperProjectedIntersection->setSecondPoint( secondIntersectionPoint );
            // linia de background
            m_backgroundUpperProjectedIntersection->setFirstPoint( firstIntersectionPoint );
            m_backgroundUpperProjectedIntersection->setSecondPoint( secondIntersectionPoint );

            m_2DViewer->getDrawer()->showGroup("ReferenceLines");
        }
        else
        {
            m_2DViewer->getDrawer()->hideGroup("ReferenceLines");
        }

        QList< QVector<double> > lowerPlaneBounds = referencePlane->getLowerBounds();
        numberOfIntersections = this->getIntersections( lowerPlaneBounds.at(0), lowerPlaneBounds.at(1), lowerPlaneBounds.at(2), lowerPlaneBounds.at(3), localizerPlane, firstIntersectionPoint, secondIntersectionPoint );

        // un cop tenim les interseccions nomes cal projectar-les i pintar la linia
        DEBUG_LOG(" ======== Nombre d'interseccions entre plans: " +  QString::number( numberOfIntersections ) );
        if( numberOfIntersections == 2 )
        {
            m_2DViewer->projectDICOMPointToCurrentDisplayedImage( firstIntersectionPoint, firstIntersectionPoint );
            m_2DViewer->projectDICOMPointToCurrentDisplayedImage( secondIntersectionPoint, secondIntersectionPoint );

            // linia discontinua
            m_lowerProjectedIntersection->setFirstPoint( firstIntersectionPoint );
            m_lowerProjectedIntersection->setSecondPoint( secondIntersectionPoint );
            // linia de background
            m_backgroundLowerProjectedIntersection->setFirstPoint( firstIntersectionPoint );
            m_backgroundLowerProjectedIntersection->setSecondPoint( secondIntersectionPoint );

            m_2DViewer->getDrawer()->showGroup("ReferenceLines");
        }
        else
        {
            m_2DViewer->getDrawer()->hideGroup("ReferenceLines");
            // si no hi ha cap intersecció apliquem el pla directament, "a veure què"
            //TODO això és per debug ONLY!!
//             projectPlane( referencePlane );
        }
    }
}

void ReferenceLinesTool::projectPlane(ImagePlane *planeToProject)
{
    QList< QVector<double> > planeBounds = planeToProject->getCentralBounds();
    double projectedVertix1[3],projectedVertix2[3],projectedVertix3[3],projectedVertix4[3];

    m_2DViewer->projectDICOMPointToCurrentDisplayedImage( (double *)planeBounds.at(0).data(), projectedVertix1 );
    m_2DViewer->projectDICOMPointToCurrentDisplayedImage( (double *)planeBounds.at(1).data(), projectedVertix2 );
    m_2DViewer->projectDICOMPointToCurrentDisplayedImage( (double *)planeBounds.at(2).data(), projectedVertix3 );
    m_2DViewer->projectDICOMPointToCurrentDisplayedImage( (double *)planeBounds.at(3).data(), projectedVertix4 );

    // donem els punts al poligon a dibuixar
    m_projectedReferencePlane->setVertix( 0, projectedVertix1 );
    m_projectedReferencePlane->setVertix( 1, projectedVertix2 );
    m_projectedReferencePlane->setVertix( 2, projectedVertix3 );
    m_projectedReferencePlane->setVertix( 3, projectedVertix4 );
    m_2DViewer->getDrawer()->showGroup("ReferenceLines");
}

int ReferenceLinesTool::getIntersections( QVector<double> tlhc, QVector<double> trhc, QVector<double> brhc, QVector<double> blhc, ImagePlane *localizerPlane, double firstIntersectionPoint[3], double secondIntersectionPoint[3] )
{
    double t;
    int numberOfIntersections = 0;
    double localizerNormalVector[3], localizerOrigin[3];
    localizerPlane->getNormalVector( localizerNormalVector );
    localizerPlane->getOrigin( localizerOrigin );
    if( vtkPlane::IntersectWithLine( (double *)tlhc.data(), (double *)trhc.data(), localizerNormalVector, localizerOrigin, t, firstIntersectionPoint ) )
    {
        numberOfIntersections = 1;

        if( vtkPlane::IntersectWithLine( (double *)brhc.data(), (double *)blhc.data(), localizerNormalVector, localizerOrigin, t, secondIntersectionPoint ) )
        {
            numberOfIntersections = 2;
        }
    }
    else if( vtkPlane::IntersectWithLine( (double *)trhc.data(), (double *)brhc.data(), localizerNormalVector, localizerOrigin, t, firstIntersectionPoint ) )
    {
        numberOfIntersections = 1;

        if( vtkPlane::IntersectWithLine( (double *)blhc.data(), (double *)tlhc.data(), localizerNormalVector, localizerOrigin, t, secondIntersectionPoint ) )
        {
            numberOfIntersections = 2;
        }
    }
    return numberOfIntersections;
}

void ReferenceLinesTool::updateFrameOfReference()
{
    Q_ASSERT( m_2DViewer->getInput() ); // hi ha d'haver input per força
    Series *series = m_2DViewer->getInput()->getSeries();
    if( series )
    {
        // ens guardem el nostre
        m_myFrameOfReferenceUID = series->getFrameOfReferenceUID();
        // i actualitzem el de les dades
        m_myData->setFrameOfReferenceUID( m_myFrameOfReferenceUID );
    }
    else
    {
        DEBUG_LOG("EL nou volum no té series NUL!");
    }
}

void ReferenceLinesTool::updateImagePlane()
{
    m_myData->setImagePlane( m_2DViewer->getCurrentImagePlane() );
}

void ReferenceLinesTool::refreshReferenceViewerData()
{
    // si es projectaven plans sobre el nostre drawer, les amaguem
    m_2DViewer->getDrawer()->hideGroup("ReferenceLines");
    if( m_2DViewer->getInput() )
    {
        updateFrameOfReference();
        updateImagePlane();
    }
}

}
