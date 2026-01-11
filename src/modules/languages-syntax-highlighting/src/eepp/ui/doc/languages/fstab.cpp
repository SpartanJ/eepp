#include <eepp/ui/doc/languages/fstab.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addFstab() {

	return SyntaxDefinitionManager::instance()->add(

		{ "fstab",
		  { "fstab" },
		  {
			  { { "^#.*$" }, "comment" },
			  { { "[=/:.,]+" }, "operator" },
			  { { "/.*/" }, "string" },
			  { { "#" }, "operator" },
			  { { "%w-%-%w-%-%w-%-%w-%-%w- " }, "string" },
			  { { "%d+%.%d+%.%d+%.%d+" }, "string" },
			  { { " %d+ " }, "number" },
			  { { "[%w_]+" }, "symbol" },

		  },
		  {
			  { "vfat", "type" },	   { "ecryptfs", "type" },	  { "devpts", "type" },
			  { "squashfs", "type" },  { "LABEL", "keyword" },	  { "aufs", "type" },
			  { "hugetlbfs", "type" }, { "proc", "type" },		  { "btrfs", "type" },
			  { "binder", "type" },	   { "binfmt_misc", "type" }, { "securityfs", "type" },
			  { "cgroup", "type" },	   { "cgroup2", "type" },	  { "sockfs", "type" },
			  { "swap", "type" },	   { "pipefs", "type" },	  { "sysfs", "type" },
			  { "none", "literal" },   { "mqueue", "type" },	  { "configfs", "type" },
			  { "UUID", "keyword" },   { "rpc_pipefs", "type" },  { "hfs", "type" },
			  { "bdev", "type" },	   { "fusectl", "type" },	  { "ntfs", "type" },
			  { "tmpfs", "type" },	   { "hfsplus", "type" },	  { "devtmpfs", "type" },
			  { "fuseblk", "type" },   { "debugfs", "type" },	  { "autofs", "type" },
			  { "cpuset", "type" },	   { "ufs", "type" },		  { "nfs", "type" },
			  { "jfs", "type" },	   { "fuse", "type" },		  { "bpf", "type" },
			  { "nfs4", "type" },	   { "pstore", "type" },	  { "ext2", "type" },
			  { "ext3", "type" },	   { "tracefs", "type" },	  { "minix", "type" },
			  { "msdos", "type" },	   { "xfs", "type" },		  { "ext4", "type" },
			  { "ramfs", "type" },	   { "nfsd", "type" },		  { "qnx4", "type" },

		  },
		  "#",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
