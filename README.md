# SSDP

The [Simple/Slick/Stupid/Smart/../Slimy/Sexy] Sky Dome Projector (SSDP)
(have not decided yet which adjective fits best, there is just so many) 
is a library for modeling irradiance taking into accound the local 
topography. It implements a hexagonal mesh for the sky and the Perez 
all weather sky model. For the topography it works with a point cloud 
of x,y,z coordinates or a regular grid. It can compute the horizon as 
seen from a specific location in the topography and can project the 
modelled sky dome onto a surface with a specified tilt and orientation. 
The surface location and orientation may be adapted to the topography 
(e.g. specify height above the surface and adapt the orientation to the 
surface normal).

