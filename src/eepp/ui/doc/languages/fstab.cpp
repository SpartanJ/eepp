#include <eepp/ui/doc/languages/fstab.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

void addFstab() {

	SyntaxDefinitionManager::instance()->add(

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
			  { { "%s+" }, "normal" },
			  { { "%w+%f[%s]" }, "normal" },

		  },
		  {
			  { "vfat", "keyword2" },		 { "ecryptfs", "keyword2" },
			  { "devpts", "keyword2" },		 { "squashfs", "keyword2" },
			  { "LABEL", "keyword" },		 { "aufs", "keyword2" },
			  { "hugetlbfs", "keyword2" },	 { "proc", "keyword2" },
			  { "btrfs", "keyword2" },		 { "binder", "keyword2" },
			  { "binfmt_misc", "keyword2" }, { "securityfs", "keyword2" },
			  { "cgroup", "keyword2" },		 { "cgroup2", "keyword2" },
			  { "sockfs", "keyword2" },		 { "swap", "keyword2" },
			  { "pipefs", "keyword2" },		 { "sysfs", "keyword2" },
			  { "none", "literal" },		 { "mqueue", "keyword2" },
			  { "configfs", "keyword2" },	 { "UUID", "keyword" },
			  { "rpc_pipefs", "keyword2" },	 { "hfs", "keyword2" },
			  { "bdev", "keyword2" },		 { "fusectl", "keyword2" },
			  { "ntfs", "keyword2" },		 { "tmpfs", "keyword2" },
			  { "hfsplus", "keyword2" },	 { "devtmpfs", "keyword2" },
			  { "fuseblk", "keyword2" },	 { "debugfs", "keyword2" },
			  { "autofs", "keyword2" },		 { "cpuset", "keyword2" },
			  { "ufs", "keyword2" },		 { "nfs", "keyword2" },
			  { "jfs", "keyword2" },		 { "fuse", "keyword2" },
			  { "bpf", "keyword2" },		 { "nfs4", "keyword2" },
			  { "pstore", "keyword2" },		 { "ext2", "keyword2" },
			  { "ext3", "keyword2" },		 { "tracefs", "keyword2" },
			  { "minix", "keyword2" },		 { "msdos", "keyword2" },
			  { "xfs", "keyword2" },		 { "ext4", "keyword2" },
			  { "ramfs", "keyword2" },		 { "nfsd", "keyword2" },
			  { "qnx4", "keyword2" },

		  },
		  "#",
		  {}

		} );
}

}}}} // namespace EE::UI::Doc::Language
