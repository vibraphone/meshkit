/*
This section of the page provides templates for adding tables, and rows.

***************
* Empty Table *
***************

----------8<-----------------------------------------------------------8<----------
<center><table width=80% bgcolor=black style="text-align: center;" border="0" cellpadding="5" cellspacing="2"><tbody>

<tr bgcolor=darkgray> <td>
Algorithm</td><td>
Related Examples</td><td>
Related Tests</td><td>
Brief Info</td></tr>

<tr bgcolor=white> <td colspan="4"> <big><b>  0D Algorithms </b></big> </td> </tr>

<tr bgcolor=white> <td colspan="4"> <big><b>  1D Algorithms </b></big> </td> </tr>

<tr bgcolor=white> <td colspan="4"> <big><b>  2D Algorithms </b></big> </td> </tr>

<tr bgcolor=white> <td colspan="4"> <big><b>  3D Algorithms </b></big> </td> </tr>

</tbody></table></center>
----------8<-----------------------------------------------------------8<----------

***************
* Table Entry *
***************

Color notes:
springgreen is for LGPL-compatible license - Bundled with Meshkit
skyblue is for LGPL-compatible license - External
goldenrod is for Propietary or GPL liscence External

Subpage note:
If an algorithm's description needs to be linked twice ONLY use \subpage once, for any occurance after the first, use \ref

----------8<-----------------------------------------------------------8<----------
<tr bgcolor=springgreen OR goldenrod OR skyblue>  <td> \endhtmlonly
<link to description i.e. \subpage alg_page>      \htmlonly </td> <td> \endhtmlonly
<link to example i.e. \ref example_page>          \htmlonly </td> <td> \endhtmlonly
<link to test i.e. \ref test_page>                \htmlonly </td> <td> \endhtmlonly
<a brief description, objective, short>           \htmlonly </td> </tr>
----------8<-----------------------------------------------------------8<----------

*/

/*!
\page meshkitalgorithms MeshKit Algorithms

\htmlonly

<center>
<table width=80% bgcolor=black style="text-align: center;" border="0" cellpadding="5" cellspacing="2"><tbody><tr>
<td bgcolor=darkgray> Key: </td>
<td bgcolor=springgreen> LGPL-compatible license <br> Bundled with MeshKit </td>
<td bgcolor=skyblue> LGPL-compatible license <br> External </td>
<td bgcolor=goldenrod> Proprietary or GPL license <br> External </td>
</tr></tbody></table></center>









<br><br><big><big>  Mesh Generation Algorithms  </big></big><br><br>

<center><table width=80% bgcolor=black style="text-align: center;" border="0" cellpadding="5" cellspacing="2"><tbody>

<tr bgcolor=darkgray> <td>
algorithm</td><td>
related examples</td><td>
related tests</td><td>
brief info</td></tr>

<tr bgcolor=white> <td colspan="4"> <big><b>  0D algorithms </b></big> </td> </tr>

<tr bgcolor=springgreen>                          <td> \endhtmlonly
\subpage algorithm_vertexmesher                   \htmlonly </td> <td> \endhtmlonly
(none)                                            \htmlonly </td> <td> \endhtmlonly
(none)                                            \htmlonly </td> <td> \endhtmlonly
<a brief description, objective, short>           \htmlonly </td> </tr>

<tr bgcolor=white> <td colspan="4"> <big><b>  1D algorithms </b></big> </td> </tr>

<tr bgcolor=springgreen>                          <td> \endhtmlonly
\subpage algorithm_edgemesher                     \htmlonly </td> <td> \endhtmlonly
(none)                                            \htmlonly </td> <td> \endhtmlonly
(none)                                            \htmlonly </td> <td> \endhtmlonly
<a brief description, objective, short>           \htmlonly </td> </tr>

<tr bgcolor=white> <td colspan="4"> <big><b>  2D algorithms </b></big> </td> </tr>

<tr bgcolor=goldenrod>                            <td> \endhtmlonly
\subpage algorithm_camaltriadvance                \htmlonly </td> <td> \endhtmlonly
\ref example_camaltriadvance.cpp                  \htmlonly </td> <td> \endhtmlonly
\ref test_camaltriadvance.cpp                     \htmlonly </td> <td> \endhtmlonly
<a brief description, objective, short>           \htmlonly </td> </tr>

<tr bgcolor=springgreen>                          <td> \endhtmlonly
\subpage algorithm_copymesh                       \htmlonly </td> <td> \endhtmlonly
\ref example_copymesh.cpp                         \htmlonly </td> <td> \endhtmlonly
\ref test_copymesh.cpp                            \htmlonly </td> <td> \endhtmlonly
<a brief description, objective, short>           \htmlonly </td> </tr>

<tr bgcolor=springgreen>                          <td> \endhtmlonly
\subpage algorithm_jaalquad                       \htmlonly </td> <td> \endhtmlonly
\ref example_jaalquad.cpp                         \htmlonly </td> <td> \endhtmlonly
\ref test_jaalquad.cpp                            \htmlonly </td> <td> \endhtmlonly
<a brief description, objective, short>           \htmlonly </td> </tr>

<tr bgcolor=white> <td colspan="4"> <big><b>  3D algorithms </b></big> </td> </tr>

<tr bgcolor=goldenrod>                            <td> \endhtmlonly
\subpage algorithm_camaltetmesher                 \htmlonly </td> <td> \endhtmlonly
\ref example_camaltetmesher.cpp                   \htmlonly </td> <td> \endhtmlonly
\ref test_camaltetmesher.cpp                      \htmlonly </td> <td> \endhtmlonly
<a brief description, objective, short>           \htmlonly </td> </tr>

<tr bgcolor=springgreen>                          <td> \endhtmlonly
\ref algorithm_copymesh                           \htmlonly </td> <td> \endhtmlonly
\ref example_copymesh.cpp                         \htmlonly </td> <td> \endhtmlonly
\ref test_copymesh.cpp                            \htmlonly </td> <td> \endhtmlonly
<a brief description, objective, short>           \htmlonly </td> </tr>

<tr bgcolor=springgreen>                          <td> \endhtmlonly
\subpage algorithm_extrudemesh                    \htmlonly </td> <td> \endhtmlonly
\ref example_extrudemesh.cpp                      \htmlonly </td> <td> \endhtmlonly
\ref test_extrudemesh.cpp                         \htmlonly </td> <td> \endhtmlonly
<a brief description, objective, short>           \htmlonly </td> </tr>

<tr bgcolor=springgreen>                          <td> \endhtmlonly
\subpage algorithm_ebmesh                         \htmlonly </td> <td> \endhtmlonly
\ref example_ebmesh.cpp                           \htmlonly </td> <td> \endhtmlonly
\ref test_ebmesh.cpp                              \htmlonly </td> <td> \endhtmlonly
<a brief description, objective, short>           \htmlonly </td> </tr>

<tr bgcolor=skyblue>                              <td> \endhtmlonly
\subpage algorithm_ngtetmesher                    \htmlonly </td> <td> \endhtmlonly
\ref example_ngtetmesher.cpp                      \htmlonly </td> <td> \endhtmlonly
\ref test_ngtetmesher.cpp                         \htmlonly </td> <td> \endhtmlonly
<a brief description, objective, short>           \htmlonly </td> </tr>

<tr bgcolor=springgreen>                          <td> \endhtmlonly
\subpage algorithm_onetooneswept                  \htmlonly </td> <td> \endhtmlonly
\ref example_onetooneswept.cpp                    \htmlonly </td> <td> \endhtmlonly
\ref test_onetooneswept.cpp                       \htmlonly </td> <td> \endhtmlonly
<a brief description, objective, short>           \htmlonly </td> </tr>

<tr bgcolor=springgreen>                          <td> \endhtmlonly
\subpage algorithm_scdmesh                        \htmlonly </td> <td> \endhtmlonly
\ref example_scdmesh.cpp                          \htmlonly </td> <td> \endhtmlonly
\ref test_scdmesh.cpp                             \htmlonly </td> <td> \endhtmlonly
<a brief description, objective, short>           \htmlonly </td> </tr>

</tbody></table></center>









<br><br><big><big>  Geometry Algorithms </big></big><br><br>

<center><table width=80% bgcolor=black style="text-align: center;" border="0" cellpadding="5" cellspacing="2"><tbody>

<tr bgcolor=darkgray> <td>
algorithm</td><td>
related examples</td><td>
related tests</td><td>
brief info</td></tr>

<tr bgcolor=white> <td colspan="4"> <big><b>  2D algorithms </b></big> </td> </tr>

<tr bgcolor=springgreen>                          <td> \endhtmlonly
\subpage algorithm_assygen                        \htmlonly </td> <td> \endhtmlonly
\ref example_assygen.cpp                          \htmlonly </td> <td> \endhtmlonly
\ref test_assygen.cpp                             \htmlonly </td> <td> \endhtmlonly
<a brief description, objective, short>           \htmlonly </td> </tr>

<tr bgcolor=springgreen>                          <td> \endhtmlonly
\subpage algorithm_copygeom                       \htmlonly </td> <td> \endhtmlonly
\ref example_copygeom.cpp                         \htmlonly </td> <td> \endhtmlonly
\ref test_copygeom.cpp                            \htmlonly </td> <td> \endhtmlonly
<a brief description, objective, short>           \htmlonly </td> </tr>

<tr bgcolor=white> <td colspan="4"> <big><b>  3D algorithms </b></big> </td> </tr>

<tr bgcolor=springgreen>                          <td> \endhtmlonly
\ref algorithm_assygen                            \htmlonly </td> <td> \endhtmlonly
\ref example_assygen.cpp                          \htmlonly </td> <td> \endhtmlonly
\ref test_assygen.cpp                             \htmlonly </td> <td> \endhtmlonly
<a brief description, objective, short>           \htmlonly </td> </tr>

<tr bgcolor=springgreen>                          <td> \endhtmlonly
\ref algorithm_copygeom                           \htmlonly </td> <td> \endhtmlonly
\ref example_copygeom.cpp                         \htmlonly </td> <td> \endhtmlonly
\ref test_copygeom.cpp                            \htmlonly </td> <td> \endhtmlonly
<a brief description, objective, short>           \htmlonly </td> </tr>

<tr bgcolor=springgreen>                          <td> \endhtmlonly
\subpage algorithm_meshoptemplate                 \htmlonly </td> <td> \endhtmlonly
\ref example_meshoptemplate.cpp                   \htmlonly </td> <td> \endhtmlonly
\ref test_meshoptemplate.cpp                      \htmlonly </td> <td> \endhtmlonly
<a brief description, objective, short>           \htmlonly </td> </tr>

</tbody></table></center>









<br><br><big><big>  Mesh Modification Algorithms  </big></big><br><br>

<center><table width=80% bgcolor=black style="text-align: center;" border="0" cellpadding="5" cellspacing="2"><tbody>

<tr bgcolor=darkgray> <td>
algorithm</td><td>
related examples</td><td>
related tests</td><td>
brief info</td></tr>

<tr bgcolor=white> <td colspan="4"> <big><b>  2D algorithms </b></big> </td> </tr>

<tr bgcolor=goldenrod>                            <td> \endhtmlonly
\subpage algorithm_mergemesh                      \htmlonly </td> <td> \endhtmlonly
(none)                                            \htmlonly </td> <td> \endhtmlonly
(none)                                            \htmlonly </td> <td> \endhtmlonly
<a brief description, objective, short>           \htmlonly </td> </tr>

<tr bgcolor=skyblue>                              <td> \endhtmlonly
\subpage algorithm_mesquiteopt                    \htmlonly </td> <td> \endhtmlonly
\ref example_mesquiteopt.cpp                      \htmlonly </td> <td> \endhtmlonly
\ref test_mesquiteopt.cpp                         \htmlonly </td> <td> \endhtmlonly
<a brief description, objective, short>           \htmlonly </td> </tr>

<tr bgcolor=springgreen>                          <td> \endhtmlonly
\subpage algorithm_qslimmesher                    \htmlonly </td> <td> \endhtmlonly
\ref example_qslimmesher.cpp                      \htmlonly </td> <td> \endhtmlonly
\ref test_qslimmesher.cpp                         \htmlonly </td> <td> \endhtmlonly
<a brief description, objective, short>           \htmlonly </td> </tr>

<tr bgcolor=white> <td colspan="4"> <big><b>  3D algorithms </b></big> </td> </tr>

<tr bgcolor=goldenrod>                            <td> \endhtmlonly
\ref algorithm_mergemesh                          \htmlonly </td> <td> \endhtmlonly
(none)                                            \htmlonly </td> <td> \endhtmlonly
(none)                                            \htmlonly </td> <td> \endhtmlonly
<a brief description, objective, short>           \htmlonly </td> </tr>

<tr bgcolor=skyblue>                              <td> \endhtmlonly
\ref algorithm_mesquiteopt                        \htmlonly </td> <td> \endhtmlonly
\ref example_mesquiteopt.cpp                      \htmlonly </td> <td> \endhtmlonly
\ref test_mesquiteopt.cpp                         \htmlonly </td> <td> \endhtmlonly
<a brief description, objective, short>           \htmlonly </td> </tr>

</tbody></table></center>

\endhtmlonly

*/
