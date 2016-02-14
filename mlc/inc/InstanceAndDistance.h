
#ifndef InstanceAndDistance_h
#define InstanceAndDistance_h

#include <Pix.h>
#include <basics.h>


class InstanceAndDistance {
public:   
   Real distance;
   Pix pix;
   InstanceAndDistance() { distance = -REAL_MAX; pix = NULL; }
   Bool operator<(const InstanceAndDistance& i) const {
      return distance < i.distance;
   }
   Bool operator>(const InstanceAndDistance& i) const {
      return distance > i.distance;
   }
   Bool operator>=(const InstanceAndDistance& i) const {
      return distance >= i.distance;
   }
   Bool operator<=(const InstanceAndDistance& i) const {
      return distance <= i.distance;
   }      
   Bool operator==(const InstanceAndDistance& i) const  {
      return distance == i.distance;
   }
   InstanceAndDistance& operator=(const InstanceAndDistance& i) {
      if( this != &i) {
	 distance = i.distance;
	 pix = i.pix;
      }
      return *this;
   }

};   
#endif



