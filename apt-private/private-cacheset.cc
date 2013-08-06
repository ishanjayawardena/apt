#include <apt-pkg/cachefile.h>
#include <apt-pkg/pkgcache.h>
#include <apt-pkg/depcache.h>
#include <apt-pkg/strutl.h>

#include "private-cacheset.h"

bool GetLocalitySortedVersionSet(pkgCacheFile &CacheFile, 
                                 LocalitySortedVersionSet &output_set,
                                 OpProgress &progress)
{
    Matcher null_matcher = Matcher();
    return GetLocalitySortedVersionSet(CacheFile, output_set, 
                                       null_matcher, progress);
}

bool GetLocalitySortedVersionSet(pkgCacheFile &CacheFile, 
                                 LocalitySortedVersionSet &output_set,
                                 Matcher &matcher,
                                 OpProgress &progress)
{
   pkgCache *Cache = CacheFile.GetPkgCache();
   pkgDepCache *DepCache = CacheFile.GetDepCache();

   int Done=0;
   progress.SubProgress(Cache->Head().PackageCount, _("Sorting"));
   for (pkgCache::PkgIterator P = Cache->PkgBegin(); P.end() == false; ++P)
   {
      if (Done%500 == 0)
         progress.Progress(Done);
      Done++;

      if ((matcher)(P) == false)
         continue;

      // exclude virtual pkgs
      if (P.VersionList() == 0)
         continue;
      pkgDepCache::StateCache &state = (*DepCache)[P];
      if (_config->FindB("APT::Cmd::Installed") == true)
      {
         if (P.CurrentVer() != NULL)
         {
            output_set.insert(P.CurrentVer());
         }
      }
      else if (_config->FindB("APT::Cmd::Upgradable") == true)
      {
         if(P.CurrentVer() && state.Upgradable())
         {
             pkgPolicy *policy = CacheFile.GetPolicy();
             output_set.insert(policy->GetCandidateVer(P));
         }
      }
      else 
      {
         pkgPolicy *policy = CacheFile.GetPolicy();
         output_set.insert(policy->GetCandidateVer(P));
      }
   }
   progress.Done();
   return true;
}
