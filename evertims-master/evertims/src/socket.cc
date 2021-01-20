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
 

#include <iostream>

#include "socket.h"

//
// Open the new socket connection
//
void Socket::openSocket()
{
    int err;
    
#ifdef WIN32
    erreur=WSAStartup(MAKEWORD(2,2),&initialisation_win32);
    if( erreur!=0 ){ printf("\nSorry, can't initialize Winsock due to error : %d %d",erreur,WSAGetLastError()); }
    else{ printf("\nWSAStartup  : OK"); }
#endif
    
    m_socket_id = socket(AF_INET,SOCK_DGRAM,0);
    
    if( m_socket_id == -1 ){ printf("Sorry, can't create socket due to error\n"); }
    else{ printf("socket      : OK\n"); }
}

//
// Open the new socket connection
//
Socket::Socket(int port)
{
    memset((char *)&m_sockaddr,0,sizeof(m_sockaddr));
    
    m_sockaddr.sin_family = AF_INET;
    m_sockaddr.sin_port = htons(port); // Listen on the Port
    m_sockaddr.sin_addr.s_addr = INADDR_ANY; // Ecoute sur toutes les IP locales
    
    openSocket();
    
    // Bind the socket to an IP and a port for Listening (Lie la socket � une ip et un port d'�coute )
    int err = bind( m_socket_id,
                   (struct sockaddr*)&m_sockaddr,
                   sizeof(m_sockaddr) );
    
    if ( err != 0 ){ printf("Error, code = %d\n", errno); }
    else{ printf("bind        : OK\n"); }
}

//
// Open the new socket connection to a given host
//
Socket::Socket(char *host)
{
    struct hostent *hostinfo;
    int error;
    int port;
    
    char *s = strchr(host, ':');
    if( s == NULL )
    {
        fprintf(stderr, "You should have a \':\'-character between the hostname and the port number: %s. Please, try again!\n", host);
        return;
    }
    port = atoi(s+1);
    *s = '\0';
    
    memset((char *)&m_sockaddr,0,sizeof(m_sockaddr));
    m_sockaddr.sin_family = AF_INET;
    m_sockaddr.sin_port = htons(port); // Listen on the Port
    
    hostinfo=gethostbyname(host);
    if( hostinfo==NULL )
    {
        fprintf( stdout , "Unknown host %s \n", host);
        return;
    }
    
    m_sockaddr.sin_addr=*(struct in_addr*)hostinfo->h_addr;
    
    openSocket();
}


//
// close the socket
//
Socket::~Socket()
{
    int error;
    
    // ********************************************************
    // Closing corresponding socket � command socket()
    // ********************************************************
#ifdef _WIN32
    erreur=closesocket(id_socket_in);
#else
    error=close(m_socket_id);
#endif
    if( error!=0 ){ printf("\nSorry, can't free socket due to error : %d",error); }
    else{ printf("\nclosesocket : OK"); }
    
#ifdef _WIN32
    // ******************************************************
    // Neatly quit the winsock opened with WSAStartup command
    // ******************************************************
    erreur=WSACleanup(); // A appeler autant de fois qu'il a �t� ouvert.
    if( erreur!=0 )
    {
        printf("\nDesole, je ne peux pas liberer winsock du a l'erreur : %d %d",erreur,WSAGetLastError());
    }
    else{ printf("\nWSACleanup  : OK"); }
    
#endif
}

//
// Read the next data package from the socket
//
int Socket::read(char *buffer)
{
    socklen_t fromlen=sizeof(struct sockaddr);
    int numCharacters=recvfrom(m_socket_id,
                               buffer,
                               1024,
                               0,
                               (struct sockaddr*)&m_sockaddr,
                               &fromlen);
    buffer[numCharacters]=0; // Allows to close the table after data content, because recvfrom function doesn't
    
    //  printf("M: %s.\n", buffer);
    
    return numCharacters;
}

//
// send the data
//
void Socket::write(int len, char *message)
{
    int numCharacters;
    
    socklen_t tolen = sizeof(struct sockaddr_in);
    
    //  printf("SEND: <%s>\n", message);
    
    numCharacters = sendto(m_socket_id,
                           message,
                           len,
                           0,
                           (struct sockaddr*)&m_sockaddr,
                           tolen);
    
    if( numCharacters == -1 ){ printf ( "Send failed! Code = %d\n", errno); }
    //  else
    //    printf ( "Send OK.\n");
}

