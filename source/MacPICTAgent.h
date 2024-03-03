#pragma once
#include "shared.h"

#ifdef MACINTOSH
/*
for Mac, there will need to be a MacPICT Agent
since GetMedia returns PICT instead of a DIB on Mac
and the Director built in PICT Agent is just a placeholder
(never actually returns anything)
look at the WinBMP Agent for example implementation of an Agent
this is not required to be crossplatform, so ideally, this
would use the official Mac method of reading PICT, whatever that is
the goal would be to provide both 24-bit and 32-bit versions of the image
that match the pixel formats described in mixpix.h
(other formats not necessary, the Writers can downscale to 1/2/4/8/16-bit)
I'm not familiar with how this is done on Mac, but
the XDK docs suggest this is possible by rendering the PICT into a GWorld by
using DrawPicture, then you'll have direct access to the pixels
(see Tips, Guidelines and Gotchas under Multimedia Developer's Guide)
this would be the ideal I think, though failing that, we could
potentially bring on ImageMagick to do it (free software licenses permitting)
*/
#endif