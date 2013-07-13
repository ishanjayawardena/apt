#ifndef APT_PRIVATE_CACHEFILE_H
#define APT_PRIVATE_CACHEFILE_H

#include <apt-pkg/cachefile.h>
#include <apt-pkg/progress.h>


// class CacheFile - Cover class for some dependency cache functions	/*{{{*/
// ---------------------------------------------------------------------
/* */
class CacheFile : public pkgCacheFile
{
   static pkgCache *SortCache;
   static int NameComp(const void *a,const void *b);
   
   public:
   pkgCache::Package **List;
   
   void Sort();
   bool CheckDeps(bool AllowBroken = false);
   bool BuildCaches(bool WithLock = true)
   {
      OpTextProgress Prog(*_config);
      if (pkgCacheFile::BuildCaches(&Prog,WithLock) == false)
	 return false;
      return true;
   }
   bool Open(bool WithLock = true) 
   {
      OpTextProgress Prog(*_config);
      if (pkgCacheFile::Open(&Prog,WithLock) == false)
	 return false;
      Sort();
      
      return true;
   };
   bool OpenForInstall()
   {
      if (_config->FindB("APT::Get::Print-URIs") == true)
	 return Open(false);
      else
	 return Open(true);
   }
   CacheFile() : List(0) {};
   ~CacheFile() {
      delete[] List;
   }
};
									/*}}}*/

#endif
