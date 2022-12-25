mingw下手动编译链接FLTK程序成功
goas
于 2013-11-25 20:36:33 发布 1664
收藏 1
版权
不管怎么说直接读makefile文件实在太麻烦了,而且编译有时还看不到情况,不过相对IDE这种东西还是方便得多的.
我在编译一个工程时命令行就一直显示"compiling ....." 无语,什么也不提示.只能手动改这个文件了.
先找到echo.因为make默认是什么也不显示的,所以"compiling....."也是用的echo命令!然后就可以看到如
"$(AA) $(BB)　$(CC) $(DD)"之类的东西在第二行了,复制这一行用下echo就行了.我用这个方法理清了这个工程的所有依赖关系.我把整个Makefile当成工程调试器了.....
另外,在mingw中直接编译链接FLTK库真是麻烦,也没有什么说明书,搞了半天,才搞清楚链接什么库是必须的
编译HELLO时会用到这几个库: -lfltk -lcomctl32 -lwsock32 -lole32 -luuid -mwindows,
comctl,wsock,ole,uuid几乎是mingw编译软件必链接的库,最后一个没有搞清楚是为什么,完整版本是:
-lfltk -lcomctl32 -lwsock32 -lole32 -luuid -O3 -Wall -Wunused -Wno-format-y2k -fno-exceptions -fno-strict-aliasing -mwindows -mno-cygwin
正如Makefile所描述的那样:
# libraries to link with:
声音: AUDIOLIBS = -lwinmm
DSO DSOFLAGS = -L. -mwindows -mno-cygwin
LD LDFLAGS = $(OPTIM) -mwindows -mno-cygwin
LD LDLIBS = -lole32 -luuid -lcomctl32 -lwsock32
GL GLDLIBS = -lglu32 -lopengl32 -lole32 -luuid -lcomctl32 -lwsock32
FLTK LINKFLTK = ../lib/libfltk.a
GL LINKFLTKGL = ../lib/libfltk_gl.a
FLTK LINKFLTKFORMS = ../lib/libfltk_forms.a ../lib/libfltk.a
图片 LINKFLTKIMG = ../lib/libfltk_images.a ../lib/libfltk.a $(IMAGELIBS)
DLL: LINKSHARED = ../lib/libfltk_images.a ../lib/libfltk_forms.a ../lib/libfltk.a $(IMAGELIBS)
图片 IMAGELIBS = -L../lib -lfltk_png -lfltk_z -lfltk_jpeg
