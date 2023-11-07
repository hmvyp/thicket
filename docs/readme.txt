Thicket is a file tree merging system that allows to build a single big directory as a result of merging several other directories. Thicket was conceived as a tool for assembling a single C/C++ source tree from multiple software componets, but it may be used for other programming languagies or even for any purpose unrelated to programming.

For program source trees it is important to handle common dependencies properly. Thus, if file tree D depends on B and C (i.e. merged from them) and if B and D both depend on A, then the result tree D shall «import» A only once. Therefore, building a result tree is not a simple recursive merge, it shall analyse the whole dependency graph (Thicket does that analysis). As a side effect we have a possibility not to «materialize» intermediate trees in the dependency chain if these intermediate trees are of no interest for us (i.e. are located out of scope of the tree that shall be built).

The main idea of Thicket is quite similar to «union» or «overlay» filesystems, but Thicket is more abstract and more strict. In case of union fs a regular file in foreground fs just hides the same regular file in background fs. In contrast with union fs, Thicket prohibits any discrepancy: all regular file «versions» shall point to the same «final source» file. 

Thicket also separates the process of buiding an abstract result tree from «materialization» technique. Indeed, the result tree may be built in various ways. 

The first way is just copying the files from source subtrees. This technique, however, has an obvious drawback: any modification of files in the «big» result tree is not propagated to the initial source files.

The second way (currently the only one implemented in Thicket) is to symlink target files or directories wherever possible. If a directory is a result of merging several «final» source directories, then such a directory is «materialized» as a «real» filesystem dirctory. However, if a directory has just a single «final source», a symlink to the terget directory is created. Regular files are always symlinked.

Currently we do not consider similar materialization technique using hard links (instead of synbolic links)  because hard links may dangerously fool source control systems. However, such a technique would be sometimes useful if all Thicket artefacts were consistently ignored by SCM.

Theoretically it is also possible to «materialize» the resulting «big tree» by mounting union (overlay) filesystems (if the operating system supports such fuilesystems, e.g. Linux). Currently we do not see any practical reason to implement union (overlay) materialization, but it seems possible. 

While building the result tree we distinguish between «real» filesystem nodes (we call them «final» nodes) and «virtual» or «reference» nodes (which represent merge results). Virtual (reference) node is a «mountpoint» node or any descendant of a «mountpoint» node. 

A mountpoint node is specified by special text file (utf-8 encoded) which has a name like 

<mountpoint_node_name>.thicket_mount.txt. 

Every line in mountpoint description file defines one «target» subtree (merge source). All target subtrees are intended to be merged into this mountpoint node. Every line in the description file is just a filesystem path, usually relative to the parent directory of the description file (may also start with ../../ to access something through the ancestor directories).

A target subtree may be a final node or may be itself a reference node. The reference chain of indirections, however, shall finaly point to some final nodes (actually existing in the filesystem). So every reference node has a union of «final targets» that are obtained by consecutive dereferencing through the reference chain. All «final targets» are final («real filesystem») nodes, i.e. directories or regular files.

The mountpoint description file says that <mountpoint_node_name> virtual node is deemed to exist here and can be materialized here (i.e. alongside the description file itself).

Note, we use the term «target» as shortcut for «reference target» (i.e. referent). At the same time a «target» may be also treated as one of the «sources» used while building (merging) the resulting «big» tree. Interchangeable use of terms «target» and «source» may be confusing, but in the current context the two terms just represent different aspects of the same thing.

Thicket is command line utility, the invokation syntax is:

<thicket executable> <options> root_path, scope_path

Available options:
-c clean only (delete all artefacts from previous merging)
-f force (do not ask before artefacts deletion)
-q quiet (implies force, do not output anything except of errors)

Parameters:

root_path   — points to the whole «universe» where all paths are resolved
scope_path  — is a relative path inside root_path that points to subtree where all nodes shall be materialized.



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



The resulting root tree is as follows:

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



