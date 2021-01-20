/*************************************************************************
 *
 * This file is part of the EVERT Library / EVERTims program for room 
 * acoustics simulation.
 *
 * This program is free software; you can redistribute it and/or modify it under 
 * the terms of the GNU General Public License as published by the Free Software 
 * Foundation; either version 2 of the License, or any later version.
 *
 * THIS PROGRAM IS DISTRIBUTED IN THE HOPE THAT IT WILL BE USEFUL; BUT WITHOUT 
 * ANY WARRANTY; WITHIOUT EVEN THE IMPLIED WARRANTY OF MERCHANTABILITY OR FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with 
 * this program; if not, see https://www.gnu.org/licenses/gpl-2.0.html or write 
 * to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
 * MA 02110-1301, USA.
 *
 * Copyright
 *
 * (C) 2007 Lauri Savioja
 * Helsinki University of Technology
 *
 * (C) 2008-2017 Markus Noisternig
 * IRCAM-CNRS-UPMC UMR9912 STMS
 *
 ************************************************************************/
 

#include <pthread.h>
#include <sys/errno.h>
#include <vector>
#include <iostream>
#include <time.h>

#ifdef __Darwin
    #include <OpenGL/gl.h>
    #include <GLUT/glut.h>
#else
    #include <GL/gl.h>
    #include <GL/glut.h>
#endif

#include "solver.h"
#include "material.h"
#include "utils.h"

// #include "graphics.h"

using namespace std;

bool calculate_signal;

Solver::Solver (int mindepth, int maxdepth, bool graphics) :
m_next_solution_flag ( false ),
m_request_for_stop ( false ),
m_start_new ( true ),
m_graphics ( graphics ),
m_current_room ( 0 ),
isLoadingNewRoom ( true ),
m_lastMappedSolutionNode ( -1 ),
m_lastAvailableSolutionNode ( -1 ),
m_newSolutionNodesAvailable ( false ),
m_next_solution ( 0 ),
m_reader ( 0 ),
m_min_depth ( mindepth ),
m_max_depth ( maxdepth )
{
    /*
     for (int idx=0 ; idx < MAX_NUM_SOLUTIONS ; idx++)
     {
     m_solutionNodes[idx].m_listener_status_major = UPDATED;
     m_solutionNodes[idx].m_geom_or_source_status = CHANGED;
     m_solutionNodes[idx].m_to_send = false;
     m_solutionNodes[idx].m_source = EL::Source();
     m_solutionNodes[idx].m_listener = EL::Listener();
     m_solutionNodes[idx].m_new_source_position = EL::Vector3 (-1000, 2000, 3434);
     m_solutionNodes[idx].m_new_listener_position = EL::Vector3 (-1000, 2000, 3434);
     m_solutionNodes[idx].m_solution = 0;
     m_solutionNodes[idx].m_writers.clear();
     }
     */
    
    EL::setStopSignalValue(false);
    
    pthread_mutex_init (&next_solution_mutex, NULL);
    
    // Start a new thread with m_next_solution.update ()
    calculate_signal = false;
    int error = pthread_create (&path_solver_thread, NULL,
                                path_solver_function, (void *)this);
    
    if( error )
    {
        switch( error )
        {
            case EAGAIN:
                COUT << "Out of resources for a new thread" << "\n";
                break;
            case EINVAL:
                COUT << "Invalid attribute" << "\n";
                break;
            case EPERM:
                COUT << "No permission to set scheduling parameters" << "\n";
                break;
            default:
                COUT << "Unknown error" << "\n";
                break;
        }
    }
    
    /*
     if (graphics)
     {
     error = pthread_create (&graphics_thread, NULL,
     graphics_function, (void *)this);
     
     if (error)
     {
     switch (error)
     {
     case EAGAIN:
     cout << "Out of resources for a new thread" << endl;
     break;
     case EINVAL:
     cout << "Invalid attribute" << endl;
     break;
     case EPERM:
     cout << "No permission to set scheduling parameters" << endl;
     break;
     default:
     cout << "Unknown error" << endl;
     break;
     }
     }
     }
     */
}

Solver::~Solver () {}

void Solver::readRoomDescription( const char* file_name, MaterialFile& materials )
{
    m_room[0].import(file_name, materials);
    m_room[1] = m_room[0];
    
    m_reader->initializeMembers(m_room[0]);
    
    return;
}

std::string solutionID( const EL::Source& source, const EL::Listener& listener )
{
    const std::string& sourceName = source.getName();
    const std::string& listenerName = listener.getName();
    
    std::string id(sourceName);
    id.append("-");
    id.append(listenerName);
    
    return id;
}

void Solver::createNewSolutionNode( const EL::Source& source, const EL::Listener& listener )
{
    int idx = m_lastAvailableSolutionNode + 1;
    
    if( idx >= MAX_NUM_SOLUTIONS )
    {
        cerr << "Too many solution nodes!!! (go and increase the MAX_NUM_SOLUTIONS constant or find the bug causing me to be here." << endl;
        return;
    }
    m_solutionNodes[idx].m_listener_status_major = UPDATED;
    m_solutionNodes[idx].m_geom_or_source_status = CHANGED;
    m_solutionNodes[idx].m_to_send = false;
    for (int i=0;i<2;i++){ m_solutionNodes[idx].m_source[i] = source; }
    for (int i=0;i<2;i++){ m_solutionNodes[idx].m_listener[i] = listener; }
    m_solutionNodes[idx].m_new_source_position = source.getPosition ();
    m_solutionNodes[idx].m_new_source_orientation = source.getOrientation ();
    m_solutionNodes[idx].m_new_listener_position = listener.getPosition ();
    m_solutionNodes[idx].m_new_listener_orientation = listener.getOrientation ();
    m_solutionNodes[idx].m_solution = 0;
    m_solutionNodes[idx].m_current = 0;
    
    std::string ID = solutionID ( source, listener );
    const char *ID_cptr = ID.c_str();
    
    for( int i = 0; i < m_writers.size(); i++ )
    {
        if( m_writers[i]->match( ID_cptr ) )
        {
            m_solutionNodes[idx].m_writers.push_back( m_writers[i] );
        }
    }
    
    COUT << "New solution node " << idx << " for " << solutionID( source, listener ) << " generated." << "\n";
    
    m_lastAvailableSolutionNode = idx;
    m_newSolutionNodesAvailable = true;
}

std::string solutionID( EL::PathSolution* solution )
{
    return solutionID( solution->getSource(), solution->getListener() );
}

void Solver::mapAvailableSolutionNodes ()
{
    int i = 0;
    
    m_newSolutionNodesAvailable = false;
    
    for( i = m_lastMappedSolutionNode + 1 ; i <= m_lastAvailableSolutionNode; i++ )
    {
        std::string id = solutionID ( m_solutionNodes[i].m_source[0], m_solutionNodes[i].m_listener[0] );
        
        if (m_solutionNodes[i].m_writers.size () > 0) { m_solutionNodeMap[id] = &(m_solutionNodes[i]); }
    }
    m_lastMappedSolutionNode = i - 1;
}

void Solver::markSourceMovementMajor( const EL::Source& source, const EL::Listener& listener )
{
    std::string id = solutionID ( source, listener );
    
    m_request_for_stop = true;
    
    //  int next = ((m_current+1)&1);
    //  int next = ((m_current+1) % 20);
    //  m_reader->getRoom(m_room[next]);
    //  m_current = next;
    
    //  cout << "The new official geometry is " << m_current << endl;
    
    t_solutionNodeIterator it = m_solutionNodeMap.find(id);
    
    if( it != m_solutionNodeMap.end() )
    {
        it->second->m_new_source_position = source.getPosition ();
        it->second->m_geom_or_source_status = CHANGED;
    }
}

void Solver::markListenerMovementMajor( const EL::Source& source, const EL::Listener& listener )
{
    std::string id = solutionID ( source, listener );
    
    t_solutionNodeIterator it = m_solutionNodeMap.find(id);
    
    const EL::Vector3& p = listener.getPosition();
    const EL::Matrix3& m = listener.getOrientation();
    
    //  cout << "Listener moved to " << p[0] << "," << p[1] << "," << p[2] << endl;
    
    if( it != m_solutionNodeMap.end() )
    {
        it->second->m_new_listener_position = p;
        it->second->m_new_listener_orientation = m;
        it->second->m_listener_status_major = CHANGED;
    }
}

void Solver::markListenerMovementMinor( const EL::Source& source, const EL::Listener& listener )
{
    std::string id = solutionID ( source, listener );
    t_solutionNodeIterator it = m_solutionNodeMap.find(id);
    
    const EL::Matrix3& ori = listener.getOrientation();
    it->second->m_new_listener_orientation = ori;
    it->second->m_listener_status_minor = CHANGED;
}

void Solver::markSourceMovementMinor( const EL::Source& source, const EL::Listener& listener )
{
    std::string id = solutionID ( source, listener );
    t_solutionNodeIterator it = m_solutionNodeMap.find(id);
    
    const EL::Matrix3& ori = source.getOrientation();
    it->second->m_new_source_orientation = ori;
    it->second->m_source_status_minor = CHANGED;
}

void Solver::interruptCalculation()
{
    // Signal the calculation thread to stop
    
    EL::setStopSignalValue(true);
    // added usleep to avoid blocking while loop in osx (postponing the complete re-write? true :)  
    while (!m_next_solution_flag){ usleep(1); };
    EL::setStopSignalValue(false);
    
    pthread_mutex_lock (&next_solution_mutex);
    m_next_solution_flag = false;
    pthread_mutex_unlock (&next_solution_mutex);
    
    COUT << "Stopped calculation with old data" << "\n";
    if (m_next_solution){ delete m_next_solution; }
}  

void Solver::createNewSolution( int depth, const EL::Source& source, const EL::Listener& listener )
{
    COUT << "Creating new solution upto level " << depth << " from geometry " << m_current_room << "\n";
    m_next_solution = new EL::PathSolution (m_room[m_current_room],
                                            source,
                                            listener,
                                            depth,
                                            true);
    
    // Signal calculation thread to start
    calculate_signal = true;
}

void Solver::markGeometryChanged ()
{
    COUT << "Geometry has changed!.";
    
    // adding the isLoadingNewRoom flag to make sure no computation will start before
    // m_reader->getRoom ( m_room[next] ) is executed AND m_current_room is updated
    // (to avoid using geometry 0 while it's geometry 1 that has just been defined.
    // would happend e.g. when markSourceMovementMajor() was called in parallel with
    // markGeometryChanged(). For the same kind of reason, isLoadingNewRoom initial
    // value is set to true at startup)
    isLoadingNewRoom = true;
    int next = (( m_current_room + 1 ) % 20);
    m_reader->getRoom ( m_room[next] );
    m_current_room = next;
    
    COUT << "The new official geometry is " << m_current_room << "\n";
    
    for( t_solutionNodeIterator it = m_solutionNodeMap.begin(); it != m_solutionNodeMap.end() ; it++ )
    {
        it->second->m_geom_or_source_status = CHANGED;
    }

    m_request_for_stop = true;
    isLoadingNewRoom = false;
}

void Solver::update ()
{
    //  cout << "Starting the update round " << calculate_signal << "," << EL::getStopSignalValue() << endl;
    //  sleep(1);
    
    if(m_newSolutionNodesAvailable){ mapAvailableSolutionNodes (); }
    
    if( m_request_for_stop && !isLoadingNewRoom )
    {
        if ( ! m_start_new ){ interruptCalculation(); }
        m_request_for_stop = false;
        m_start_new = true;
    }
    
    // Do we have a solution to be swapped into use ?
    
    if( m_next_solution_flag )
    {
        COUT << "New solution will be taken into use." << "\n";
        if( m_solutionNodeMap[ solutionID ( m_next_solution) ]->m_solution )
        {
            delete m_solutionNodeMap[ solutionID ( m_next_solution) ]->m_solution;
        }
        
        m_solutionNodeMap[ solutionID ( m_next_solution) ]->m_solution = m_next_solution;
        
        if( m_solutionNodeMap[ solutionID ( m_next_solution) ]->m_geom_or_source_status == IN_PROCESS )
        {
            m_solutionNodeMap[ solutionID ( m_next_solution) ]->m_geom_or_source_status = UPDATED;
        }
        
        m_solutionNodeMap[ solutionID ( m_next_solution) ]->m_to_send = true;
        m_solutionNodeMap[ solutionID ( m_next_solution) ]->m_listener_status_major = CHANGED;
        m_solutionNodeMap[ solutionID ( m_next_solution) ]->m_current =
        (m_solutionNodeMap[ solutionID ( m_next_solution) ]->m_current + 1)&1;
        
        COUT << "Clearing the next solution flag ( m_current = " << m_solutionNodeMap[ solutionID (m_next_solution) ]->m_current << " ) " << "\n";
        
        pthread_mutex_lock (&next_solution_mutex);
        m_next_solution_flag = false;
        pthread_mutex_unlock (&next_solution_mutex);
        
        m_start_new = true;
    }
    
    if( m_start_new )
    {
        // Loop all the solutions, and start _one_ new calculation if
        // 1) geometry or source has changed
        for( t_solutionNodeIterator it = m_solutionNodeMap.begin();
            (( it != m_solutionNodeMap.end() ) && ( m_start_new && !isLoadingNewRoom )) ; it++ )
        {
            if (it->second->m_geom_or_source_status == CHANGED)
            {
                COUT << "Geometry or source changed: " << solutionID ( it->second->m_source[0], it->second->m_listener[0] ) << "\n";
                it->second->m_geom_or_source_status = IN_PROCESS;
                int next = (it->second->m_current+1)&1;
                it->second->m_source[next].setPosition ( it->second->m_new_source_position );
                it->second->m_source[next].setOrientation ( it->second->m_new_source_orientation );
                it->second->m_listener[next].setPosition ( it->second->m_new_listener_position );
                it->second->m_listener[next].setOrientation( it->second->m_new_listener_orientation );
                createNewSolution (m_min_depth, it->second->m_source[next], it->second->m_listener[next]);
                m_start_new = false;
            }
        }
        // 2) maximum order is not reached
        for( t_solutionNodeIterator it = m_solutionNodeMap.begin();
            (( it != m_solutionNodeMap.end() ) && ( m_start_new && !isLoadingNewRoom )) ; it++ )
        {
            if (it->second->m_solution)
            {
                if (it->second->m_solution->getOrder () < m_max_depth)
                {
                    COUT << "Deepening the solution: " << solutionID ( it->second->m_source[0], it->second->m_listener[0] ) << "\n";
                    it->second->m_geom_or_source_status = IN_PROCESS;
                    int next = (it->second->m_current+1)&1;
                    it->second->m_source[next].setPosition ( it->second->m_new_source_position );
                    it->second->m_source[next].setOrientation ( it->second->m_new_source_orientation );
                    it->second->m_listener[next].setPosition ( it->second->m_new_listener_position );
                    it->second->m_listener[next].setOrientation ( it->second->m_new_listener_orientation );
                    createNewSolution (it->second->m_solution->getOrder() + 1, it->second->m_source[next], it->second->m_listener[next]);
                    m_start_new = false;
                }
            }
        }
    }
        

    // See if the listener position has changed, and update the solutions accordingly
    // and finally write the changed solutions out
    
    for( t_solutionNodeIterator it = m_solutionNodeMap.begin();
        it != m_solutionNodeMap.end() ; it++ )
    {
        if (it->second->m_solution)
        {
            //	cout << "Ready to update solution: " << solutionID ( it->second->m_solution ) << endl;
            if (it->second->m_listener_status_major == CHANGED)
            {
                COUT << "Updating the solution: " << solutionID ( it->second->m_solution ) << "\n";
                it->second->m_listener_status_major = UPDATED;
                it->second->m_listener[it->second->m_current].setPosition ( it->second->m_new_listener_position );
                it->second->m_listener[it->second->m_current].setOrientation ( it->second->m_new_listener_orientation );
                it->second->m_solution->update ();
                it->second->m_to_send = true;
            }
            
            if (it->second->m_to_send)
            {
                // Update output to all the writers of the solution node
                for (std::vector<Writer *>::iterator w = it->second->m_writers.begin();
                     w != it->second->m_writers.end(); w++)
                {
                    COUT << "Sending the solution " << solutionID ( it->second->m_solution ) << " with the " << (*w)->getType() << " protocol." << "\n";
                    (*w)->writeMajor ( it->second->m_solution );
                    it->second->m_to_send = false;
                }
            }
            
            // check if listener orientation has changed
            if (it->second->m_listener_status_minor == CHANGED)
            {
                it->second->m_listener_status_minor = UPDATED;
                it->second->m_listener[it->second->m_current].setOrientation ( it->second->m_new_listener_orientation );
                // Update output to the Auralization writer (only) of the solution node (writeReduce methods of other writers are dummies)
                for (std::vector<Writer *>::iterator w = it->second->m_writers.begin();
                     w != it->second->m_writers.end(); w++)
                {
                    COUT << "listener moved, sending additional info on solution " << solutionID ( it->second->m_solution ) << " with the " << (*w)->getType() << " protocol." << "\n";
                    (*w)->writeMinor ( it->second->m_solution, 0 );
                    it->second->m_to_send = false;
                }
            }
            
            // check if source orientation has changed
            if (it->second->m_source_status_minor == CHANGED)
            {
                it->second->m_source_status_minor = UPDATED;
                it->second->m_source[it->second->m_current].setOrientation ( it->second->m_new_source_orientation );
                // Update output to the Auralization writer (only) of the solution node (writeReduce methods of other writers are dummies)
                for (std::vector<Writer *>::iterator w = it->second->m_writers.begin();
                     w != it->second->m_writers.end(); w++)
                {
                    COUT << "source moved, sending additional info on solution " << solutionID ( it->second->m_solution ) << " with the " << (*w)->getType() << " protocol." << "\n";
                    (*w)->writeMinor ( it->second->m_solution, 1 );
                    it->second->m_to_send = false;
                }
            }
            
        }
    }
    
    m_ready_to_draw = true;
}

void *path_solver_function (void *data)
{
    while (1)
    {
        if( calculate_signal )
        {
            calculate_signal = false;
            COUT << "Thread " << pthread_self() << " beginning new calculation" << "\n";
            Solver *solver = (Solver *)data;
            solver->calculateNextSolution ();
            COUT << "Thread " << pthread_self() << " finished calculation" << "\n";
        }
        usleep( 2000 );
    }
}

