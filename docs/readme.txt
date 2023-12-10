
      Thicket file tree merging system
      ---------------------------------


Thicket is a file tree merging system that allows to build a single directory as a result of merging several other directories. It was conceived as a tool for assembling a single C/C++ source tree from multiple software components, but it may also be used for other programming languages or even for any purpose unrelated to programming.

For program source trees, we need to handle common dependencies properly. Thus, if file tree D depends on B and C (i.e., is merged from them) and if B and D, in turn, both depend on A, then the result tree D shall «import» A only once. Therefore, building a result tree is not a simple recursive merge, we need to analyze the whole dependency graph. If such analysis is done, we also have  a useful side effect: there is no need to  «materialize»  intermediate trees in the dependency chain if these intermediate trees are of no interest to us (i.e., they are located outside the tree that shall be  «materialized» ).

Note that there are some similarities with «union» or «overlay»  filesystems, but there are also some differences. In case of union fs a regular file in the foreground fs just hides the same regular file in the background fs. In contrast with union fs, Thicket prohibits any discrepancy: all regular file «versions» shall point to the same «final source» file. 

Thicket also separates the process of building an abstract result tree from «materialization» technique. Indeed, the result tree may be built in various ways. 

The first way is just to copy the files from the source subtrees. Copying, however, has the obvious drawback: any modification of files in the result tree is not propagated to the initial source files.

The second way (currently implemented in Thicket) is to symlink target files or directories wherever possible. If a directory is a result of merging several «final» source directories, then such a directory is «materialized» as a «real» filesystem directory. However, if a directory has just a single «final source», a symlink to the target directory is created. Regular files are always symlinked.

Currently, we do not consider a similar materialization technique using hard links (instead of symbolic links)  because hard links may dangerously fool source control systems. However, such a technique would be sometimes useful if all Thicket artifacts were consistently ignored by SCM.

Theoretically,  it is also possible to «materialize» the resulting «big tree» by mounting union (overlay) filesystems if the operating system supports such fuilesystems (e.g. Linux). Currently, we do not see any practical reason to implement union (overlay) materialization, but it seems possible. 

While building the result tree we distinguish between «real» filesystem nodes (we call them «final» nodes) and «virtual» or «reference» nodes (which represent merge results). A virtual (reference) node is a «mountpoint» node or any descendant of the «mountpoint» node. 

Every mountpoint node is specified by a special text file (utf-8 encoded), which has a name like 

<mountpoint_node_name>.thicket_mount.txt. 

Every line in mountpoint description file defines one «target» subtree (one merge source). All target subtrees are intended to be merged into this mountpoint node. Every line in the description file is just a filesystem path, usually relative to the parent directory of the description file (may also start with ../../ to access something through the ancestor directories).

A target subtree may be a final node or may be itself a reference node. The reference chain of indirections, however, shall finally point to some final nodes (actually existing in the filesystem). So every reference node has a union of «final targets» that are obtained by consecutive dereferencing through the reference chain. All «final targets» are final («real filesystem») nodes, i.e. directories or regular files.

The mountpoint description file says that <mountpoint_node_name> virtual node is deemed to exist here and can be materialized here (i.e. alongside the description file itself).

Note that we use the term «target» as a shortcut for «reference target» (i.e. referent). At the same time, the «target" may also be  treated as one of the «sources» used while building (merging) the resulting «big» tree. Interchangeable use of terms «target» and «source» may be confusing, but in the current context, the two terms just represent different aspects of the same thing.

Thicket is command line utility, the invokation syntax has two forms:


1) The first form:

<thicket executable> <options>  [--] root_path scope_path

Parameters (both are required):

root_path   — points to the whole «universe» where all paths are resolved
scope_path  — a path relative to root_path (!) that points to root subtree 
              where materialization shall be done.

Example:

thicket -q /absolute/path/to/my/universe relative/path/where/materialization/needed


2) The second form:

<thicket_executable> -root_lev=N <other_options> [--] scope_path

N specifies the root as parent (N=1), grandparent(N=2), etc., of the scope_path directory

Parameters:

scope_path - a path, either absolute or relative to the current(!) working directory,
             where materialization shall be done.

Example: 

thicket root_lev=2 .

In this example Thicket materializes dependencies found in the current working directory with root specified as grandfather of the current working directory 


Available options (for both forms):

-c clean only (delete all artifacts from previous merging)
-f force (do not ask before artifacts deletion)
-q quiet (implies force; does not not output anything to console except of errors)
-m=method materialization method. Available methods: 
    symlinks (default)
    copy
    mixed - copy from the outside of materialization scope, symlinks inside.




An example of root_path tree before Thicket invokation:

.
├── importedA
│   └── src
│       └── x
│           └── importedA_x_tx.txt
├── importedB
│   ├── src
│   │   ├── x
│   │   │   └── importedB_x_tx.txt
│   │   └── y
│   │       └── importedB_y_tx.txt
│   │
│   │
│   └── src_all.thicket_mount.txt
│
│     Mountpoint description content:
│     src
│     ../importedA/src
│
│
├── scope
│   ├── src
│   │   ├── a
│   │   │   ├── a1
│   │   │   │   ├── a1tx.txt
│   │   │   │   └── a1txx.txt
│   │   │   ├── a2
│   │   │   │   └── a2_tx.txt
│   │   │   └── a_tx.txt
│   │   └── b
│   │       └── A_b_sometext.txt
│   │
│   │
│   └── src_all.thicket_mount.txt
│
│     Mountpoint description content:
│     src
│     ../importedA/src
│     ../importedB/src_all



After invokation of thicket with scope parameter pointed to «scope» subdirectory (let us remind that scope parameter points to a directory where «materialization» shall be done), the resulting root tree becomes as follows:

.
├── importedA
│   └── src
│       └── x
│           └── importedA_x_tx.txt
│
│
├── importedB
│   ├── src
│   │   ├── x
│   │   │   └── importedB_x_tx.txt
│   │   └── y
│   │       └── importedB_y_tx.txt
│   │
│   │
│   └── src_all.thicket_mount.txt
│
│
├── scope
│   ├── src
│   │   ├── a
│   │   │   ├── a1
│   │   │   │   ├── a1tx.txt
│   │   │   │   └── a1txx.txt
│   │   │   ├── a2
│   │   │   │   └── a2_tx.txt
│   │   │   └── a_tx.txt
│   │   └── b
│   │       └── A_b_sometext.txt
│   ├── src_all
│   │   ├── a -> ../src/a
│   │   ├── b -> ../src/b
│   │   ├── x
│   │   │   ├── importedA_x_tx.txt -> ../../../importedA/src/x/importedA_x_tx.txt
│   │   │   └── importedB_x_tx.txt -> ../../../importedB/src/x/importedB_x_tx.txt
│   │   └── y -> ../../importedB/src/y
│   │
│   │
│   └── src_all.thicket_mount.txt


Note that almost all subdirectories in scope/src_all are materialized as symlinks except of 
scope/src_all/x that is materialized as «real» directory. That happens because the «x» directory
is itself merged from two targets (sources) and, therefore, can not be symlinked.


Building Thicket
----------------

Thicket is written in C++ using std::filesystem, so at least C++17 is needed.
gcc or clang for Linux and clang for Windows are both ok (at least clang as it bundled with Microsoft command line toolset aka «Build Tools for Visual Studio»)

To build Thicket just compile the single file: src/cornus_thicket/main.cpp
 (it is the only .cpp file in the Thicket sources, all others are just headers).
 
 Do not forget -std=c++17 compiler option if it is not default for your compiler.

