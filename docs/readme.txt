

      Thicket source tree merging system

═══════════════════════════════════════════════════



► Racionale and main features.

Thicket is a file tree merging system that allows to build a single directory as a result of merging several other directories. It was conceived as a tool for assembling a single C/C++ source tree from multiple software components, but it may also be used for other programming languages or even for any purpose unrelated to programming.

Programming and building applications become significantly more comfortable if all project source files are organized in a single source tree. However, it is usually impossible since a typical software project (even at source level) consists of several components stored in separate locations in the filesystem. Thicket creates the illusion of a single source tree that combines all files from all components. By default, Thicket uses symlinks wherever possible, so a user deals with real files in their real locations. To tell Thicket what source components shall be merged, the user writes a simple «mountpoint description» text file that just lists paths to the merged source trees. After running Thicket, a resulting directory (merge result) is created nearby.

Thicket is transitive. If some component declares its dependencies in the same manner, Thicket brings to the end user «dependencies of dependencies» and so on. In other words, any path in the mountpoint description file may refer to a real filesystem object or another mountpoint. Dependencies forming a «diamond» graph are also handled properly (common dependencies are not duplicated). 

The main idea of Thicket is quite similar to «union» or «overlay»  filesystems, but there are also some differences. In case of union fs a regular file in the foreground fs just hides the same regular file in the background fs. In contrast, Thicket prohibits any discrepancy: all regular file «versions» shall point to the same «final source» file.

 

► Materialization scope and materialization techniques.

Thicket creates all its artifacts (merge results) inside so called «materialization scope» specified by the user. Outside the materialization scope nothing is changed (although something may be analyzed if reffered from the scope).

Thicket does its work in three phases. 

1) Clearing artifacts created by previous Thicket invocation (if any).

2) Building an abstract result tree.

3) «Materialization» of the abstract result as a real filesystem tree. 


Materialization technique may vary, as the result tree may be presented to the user in different ways. 

The first way is just to copy files from source subtrees. Copying, however, has the obvious drawback: any subsequent modification of files in the result tree is not propagated back to the initial source files. If the user changes some files, the changes will be lost on the next Thicket invokation. However, «copy» materialization may be useful for build scripts that do not assume any source modifications.

The second way (preferred for development purpose) is to symlink target files or directories wherever possible. If a directory is a result of merging several «final» source directories, then such a directory is «materialized» as a «real» filesystem directory. However, if a directory has just a single «final source», a symlink to the target directory is created. Regular files are always symlinked (if they located in «materialized» directory).

Inside already symlinked directory Thicket does nothing. The user may change the content of the symlinked directory being confident that all modifications are applied to the initial location.

Symlink materialization is quite friendly to IDEs. For example, to work with Thicket in Eclipse CDT, you only need to add materialized tree to the project source tree and add (if necessary) include path to it.

If the tree is changed by Thicket, just refresh the Eclipse project tree and... that's all.

We can also imagine other materialization techniques that are not currently implemented but theoretically may be useful.

Currently, we do not consider a materialization technique based on hard links (instead of symbolic links)  because hard links may dangerously fool source control systems. However, such a technique might be sometimes useful if all Thicket artifacts are consistently ignored by SCM (and we actually recommend to gitignore them).

Theoretically,  it is also possible to materialize the resulting tree by mounting union (overlay) filesystem if the operating system supports it (e.g. Linux). Currently, we do not see any practical reason to implement union (overlay) materialization, but it seems possible. 

It would be also possible to materialize merge results not using filesystem tree structures at all. For example, we may imagine the result as a list of files intended to be consumed by make or CMake. Nothing of the above is currently planned for the foreseeable future, although the possibility remains if the need arises.




► Thicket mountpoints.

Mountpoint and mountpoint description is the core Thicket concepts. Mountpoint represents the resulting (merged) tree while mountpoint description file specifies what this resulting tree must contain.

To eleborate the concepts we need to clarify some terminology.

While constructing the abstract result tree we distinguish between «real» filesystem nodes (we call them «final» nodes) and «virtual» or «reference» nodes. Virtual (reference) node does not pre-exist before Thicket invocation. It generally represents a merge result. In particular,  there may be only one merge source, so such a virtual node may represent just a reference to another node (that explains the term «reference node»).

A virtual node may have only virtual descendants, so virtual nodes forms one or more «pure virtual» hierarchies (subtrees). The root of any «virtual» hierarchy is a mountpoint node. Mountpoint node is always located in some «real» directory.

Every mountpoint node is specified by a utf-8 encoded text file, located in the same directory where the mountpoint node itself is deemed to exist. The name of the mountpoint description file has the following form: 

<mountpoint_name>.thicket_mount.txt. 

Every line in mountpoint description file defines one «target» subtree (one merge source). All target subtrees are intended to be merged into this mountpoint node. Every line (record) in the description file is just a filesystem path, usually relative to the parent directory of the description file (may also start with ../../ to access something through the ancestor directories).

A target subtree specified in every record may be a final (real) node or may be itself a reference (virtual) node (so it may not really exist in the filesystem). The reference chain of indirections, however, shall finally point to some final nodes (actually existing in the filesystem). So every reference node has a union of «final targets» that are obtained by consecutive dereferencing through the reference chain. All «final targets» are final («real filesystem») nodes, i.e. real directories or real regular files.

The mountpoint description file says that <mountpoint_name> virtual node is deemed to exist alongside the description file and can be materialized here (if needed).

Note that we use the term «target» as a shortcut for «reference target» (i.e. referent). At the same time, the «target" may also be  treated as one of the «sources» used while building (merging) the resulting «big» tree. Interchangeable use of terms «target» and «source» may be confusing, but in the current context, the two terms just represent different aspects of the same thing.

Mountpoint description allows comments starting with #.

Let's consider an example mountpoint file named my_point.thicket_mount.txt with the following content:

║
║  # The two source trees will be merged here:
║  ../first/source/tree
║  ../second/source/tree
║

The two records (except for the comment) represent the source trees to be merged.
The relative paths are relative to the directory where the mountpoin description file resides.

Let's imagine that the first source tree (../first/source/tree) contains subdirectories A and C
and the second source tree contains subdirectories B and C.

let's also assume (for simplicity) that all the source subdirectories are «real» (actually locatetd in the filesystem) and contain only regular files.

Our mountpoint, if materialized, will contain 3 directories: A, B and C.

If symlink materialization method is used, A and B will be just a links to the corresponding sources, but C (which appears in both sources) will be materialized as a merge result, that is, as a real directory which contains links to all files located inside both incarnations of C (i.e. in ../first/source/tree/C and ../second/source/tree/C) .

Graphically, the content of materialized montpoint may be depicted as:


└─ my_point
   ├── A -> ../../first/source/tree/A
   ├── B -> ../../second/source/tree/B
   └── C (contains links to all files located in C directory of both sources)

(additional ../ in links appears because the materialized links are relative to my_point, not to containg directory as in mountpoint description).

If some file name appears in both ../first/source/tree/C and ../second/source/tree/C then Thicket generates an error (it can not merge regular files).




► Running Thicket

Thicket is a command line utility. The invocation syntax has two forms:


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

-c clean only (delete all artifacts from previous invocation)
-f force (do not ask before artifacts deletion)
-q quiet (implies force; does not output anything to console except of errors)
-m=method materialization method. Available methods: 
    symlinks (default)
    copy
    mixed - copy from the outside of materialization scope, symlinks inside.




► Building Thicket

Thicket is written in C++ using std::filesystem, so at least C++17 is needed.
gcc or clang for Linux and clang for Windows are both ok (at least clang as it bundled with Microsoft command line toolset aka «Build Tools for Visual Studio»)

To build Thicket just compile the single file: src/cornus_thicket/main.cpp
 (it is the only .cpp file in the Thicket sources, all others are just headers).

Do not forget -std=c++17 compiler option if it is not default for your compiler.

For details see shell scripts located in the "build" directory for compilers invocation examples.




► Important note.

Thicket is evolving and this readme does not cover all new features, such as extended mountpoint record syntax, new command line options, etc. See CHANGELOG.txt for the new features description.




► Thicket usage example.

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



