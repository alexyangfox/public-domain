#ifndef PIXVALUE_INCLUDED
#define PIXVALUE_INCLUDED

typedef unsigned int PIX_FIELD;
const PIX_FIELD PIX_COLOR = 1;
const PIX_FIELD PIX_MASKED = 2;
const PIX_FIELD PIX_SELECTED = 4;

/* pix values are used by the color selector too. any values given from the color selector
   have the selected member set to false */

class PixValue
{
public:
  PixValue(void); //hack. this should not exist
  PixValue(unsigned long initValue);
  PixValue(const PixValue &rhs);
  ~PixValue(void);
  const PixValue& operator=(const PixValue &rhs);

  unsigned long xcolor;
  //if true xcolor is irrelevant, and we are clearing the color (and adding it to the mask)
  bool masked;
  bool selected; //xored after drawing.
};


#endif //PIXVALUE_INCLUDED
