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
 * (C) 2004-2005 Samuli Laine
 * Helsinki University of Technology
 *
 * (C) 2008-2017 Markus Noisternig
 * IRCAM-CNRS-UPMC UMR9912 STMS
 *
 ************************************************************************/
 

#ifndef __ELBEAM_HPP
#define __ELBEAM_HPP

#if !defined (__ELPOLYGON_HPP)
    #include "elPolygon.h"
#endif
#if !defined (__ELVECTOR_HPP)
    #include "elVector.h"
#endif

namespace EL
{

class Beam
{
    
public:
    
    Beam (void);
    Beam (const Vector3& top, const Polygon& polygon);
    Beam (const Beam& beam);
    ~Beam (void);
    
    const Beam&	operator= (const Beam& beam);
    
    const Vector3& getTop (void) const { return m_top; }
    const Polygon& getPolygon (void) const { return m_polygon; }
    
    int	numPleqs (void) const
    {
        return (int)m_pleqs.size();
    }
    
    const Vector4& getPleq (int i) const
    {
        EL_ASSERT(i >= 0 && i < numPleqs());
        return m_pleqs[i];
    }
    
    //void render (const Vector3& color) const;
    EL_FORCE_INLINE bool contains (const Vector3& p) const
    {
        for (int i=0; i < numPleqs(); i++)
        {
            if (dot(p, getPleq(i)) < 0.f)
            {
                return false;
            }
        }
        return true;
    }
    
    
private:
    
    void calculatePleqs	(void);
    
    Vector3 m_top;
    Polygon m_polygon;
    std::vector<Vector4> m_pleqs;
};
    
} // namespace EL

#endif // __ELBEAM_HPP
