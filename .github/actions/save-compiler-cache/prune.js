// Each variant has one producer job. Unique upload keys let concurrent runs finish
// independently; a failed upload must never cause the last good snapshot to vanish.
module.exports = async function prune({ github, context, core }, { prefix, key, legacyPrefix }) {
  const scope = { ...context.repo, ref: context.ref };
  const list = async (cachePrefix) => github.paginate(
    github.rest.actions.getActionsCacheList,
    { ...scope, key: cachePrefix, per_page: 100 },
    (response) => response.data.actions_caches,
  );
  const generation = (cache) => {
    if (cache.ref !== context.ref || cache.size_in_bytes <= 0 || !cache.key.startsWith(prefix)) return null;
    const suffix = cache.key.slice(prefix.length);
    if (!/^\d+-\d+$/.test(suffix)) return null;
    return suffix.split('-').map(BigInt);
  };
  const newer = (a, b) => a[0] > b[0] || (a[0] === b[0] && a[1] > b[1]);

  try {
    const caches = (await list(prefix)).filter((cache) => generation(cache));
    // cache/save warns on upload failure and still exits successfully. Confirm
    // the finalized entry through the API before deleting anything.
    if (!caches.some((cache) => cache.key === key)) {
      core.info('Upload not confirmed; keeping existing compiler caches.');
      return;
    }
    const newest = caches.reduce((a, b) => newer(generation(a), generation(b)) ? a : b);
    const stale = caches.filter((cache) => newer(generation(newest), generation(cache)));
    // Migrate only this exact variant and ref after a replacement is available.
    const legacy = (await list(legacyPrefix)).filter((cache) =>
      cache.ref === context.ref && cache.key.startsWith(legacyPrefix) &&
      /^[a-f0-9]{40}-\d+$/.test(cache.key.slice(legacyPrefix.length)),
    );
    for (const cache of [...stale, ...legacy]) {
      try {
        await github.rest.actions.deleteActionsCacheById({ ...context.repo, cache_id: cache.id });
      } catch (error) {
        // A concurrent publisher may already have pruned the same old entry.
        if (error.status !== 404) throw error;
      }
    }
    core.info(`Kept compiler cache ${newest.key}; pruned ${stale.length + legacy.length} superseded snapshots.`);
  } catch (error) {
    // Cache maintenance must not prevent tests or release publication.
    core.warning(`Could not prune compiler caches: ${error.message}`);
  }
};
