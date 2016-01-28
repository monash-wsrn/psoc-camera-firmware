#!/usr/bin/python
from fuse import FUSE,Operations,FuseOSError
from subprocess import check_output
from errno import ENOENT
from stat import S_IFDIR,S_IFREG

class git_version(Operations):
    def getattr(self,path,fh=None):
        self.version=check_output(['git','describe','--abbrev=8','--dirty','--always','--tags'])
        self.version='#define GIT_VERSION "%s"\n'%self.version[:-1]
        if path=='/':
            return {'st_mode':S_IFDIR|0755}
        elif path=='/version.h':
            return {'st_mode':S_IFREG|0444,'st_size':len(self.version)}
        else: raise FuseOSError(ENOENT)
    
    def read(self,path,size,offset,fh):
        if path=='/version.h': return self.version[offset:offset+size]
        else: raise RuntimeError('wrong path: '+path)
    
    def readdir(self,path,fh):
        return ['.','..','version.h']
    
    access=None
    flush=None
    getxattr=None
    listxattr=None
    open=None
    opendir=None
    release=None
    releasedir=None
    statfs=None

fuse=FUSE(git_version(),'version',foreground=True,ro=True,nonempty=True)
