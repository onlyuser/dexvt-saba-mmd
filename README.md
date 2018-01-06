[![Build Status](https://secure.travis-ci.org/onlyuser/dexvt-saba-mmd.png)](http://travis-ci.org/onlyuser/dexvt-saba-mmd)

dexvt-saba-mmd
==============

Copyright (C) 2011-2017 <mailto:onlyuser@gmail.com>

About
-----

dexvt-saba-mmd is an MMD viewer that uses [benikabocha](https://twitter.com/benikabocha)'s excellent MMD loader.

Requirements
------------

Unix tools and 3rd party components (accessible from $PATH):

    gcc mesa-common-dev freeglut3-dev libglew-dev libglm-dev libpng-dev libpthread libBulletDynamics libBulletCollision libLinearMath

Make Targets
------------

<table>
    <tr><th> target     </th><th> action                        </th></tr>
    <tr><td> all        </td><td> make binaries                 </td></tr>
    <tr><td> test       </td><td> all + run tests               </td></tr>
    <tr><td> clean      </td><td> remove all intermediate files </td></tr>
    <tr><td> lint       </td><td> perform cppcheck              </td></tr>
    <tr><td> docs       </td><td> make doxygen documentation    </td></tr>
    <tr><td> clean_lint </td><td> remove cppcheck results       </td></tr>
    <tr><td> clean_docs </td><td> remove doxygen documentation  </td></tr>
</table>

Controls
--------

Keyboard:

<table>
    <tr><th> key   </th><th> purpose           </th></tr>
    <tr><td> f     </td><td> toggle frame rate </td></tr>
    <tr><td> h     </td><td> toggle HUD        </td></tr>
    <tr><td> space </td><td> toggle animation  </td></tr>
    <tr><td> esc   </td><td> exit              </td></tr>
</table>

References
----------

<dl>
    <dt>"benikabocha"</dt>
    <dd>https://twitter.com/benikabocha</dd>
    <dt>"OpenGL Viewer (OBJ PMD PMX)"</dt>
    <dd>https://github.com/benikabocha/saba</dd>
    <dt>"C++でMMDを読み込むライブラリを作った - Qiita"</dt>
    <dd>https://qiita.com/benikabocha/items/ae9d48e314f51746f453</dd>
</dl>

Keywords
--------

    OpenGL, glsl shader, glm
