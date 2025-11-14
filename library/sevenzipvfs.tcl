#
# using tclvfs/library/zipvfs.tcl as template
#
package provide vfs::sevenzip 0.1

package require vfs
package require sevenzip

namespace eval vfs::sevenzip {}

proc vfs::sevenzip::Execute {zipfile args} {
    Mount $zipfile $zipfile {*}$args
    source [file join $zipfile main.tcl]
}

proc vfs::sevenzip::Mount {zipfile local args} {
    set fd [sevenzip open {*}$args [::file normalize $zipfile]]
    vfs::filesystem mount $local [list ::vfs::sevenzip::handler $fd]
    vfs::RegisterMount $local [list ::vfs::sevenzip::Unmount $fd]
    return $fd
}

proc vfs::sevenzip::Unmount {fd local} {
    vfs::filesystem unmount $local
    $fd close
}

proc vfs::sevenzip::handler {zipfd cmd root relative actualpath args} {
    #::vfs::log [list $zipfd $cmd $root $relative $actualpath $args]
    if {$cmd == "matchindirectory"} {
        eval [list $cmd $zipfd $relative $actualpath] $args
    } else {
        eval [list $cmd $zipfd $relative] $args
    }
}

proc vfs::sevenzip::attributes {zipfd} { return [list "state"] }
proc vfs::sevenzip::state {zipfd args} {
    vfs::attributeCantConfigure "state" "readonly" $args
}

# If we implement the commands below, we will have a perfect
# virtual file system for zip files.

proc vfs::sevenzip::matchindirectory {zipfd path actualpath pattern type} {
    #::vfs::log [list matchindirectory $path $actualpath $pattern $type]

    set file [vfs::matchFiles $type]
    set dir [vfs::matchDirectories $type]
    if {!$file && !$dir} {
        return [list]
    }
    set listargs ""
    if {!$dir} {
        set listargs "-type f"
    } elseif {!$file} {
        set listargs "-type d"
    }
    set res [$zipfd list {*}$listargs [file join $path $pattern]]

    if {[string length $pattern] == 0} {
        if {$path eq "" || $path in $res} {
            return [list $actualpath]
        }
        return [list]
    }

    set newres [list]
    set dirname [expr {$path eq "" ? "." : $path}]
    foreach p $res {
        if {[file dirname $p] eq $dirname} {
            lappend newres [file join $actualpath [file tail $p]]
        }
    }
    #::vfs::log [list got $newres]
    return $newres
}

proc vfs::sevenzip::stat {zipfd name} {
    #::vfs::log "stat $name"
    array set sb {dev -1 uid -1 gid -1 nlink 1}

    if {$name in {{} {.}}} {
        array set sb {type directory mtime 0 size 0 mode 0777 ino -1 depth 0 name ""}
        #::vfs::log [list res [array get sb]]
        return [array get sb]
    }

    set sblist [$zipfd list -info -exact $name]
    #::vfs::log [list 7z [lindex $sblist 0]]
    if {[llength $sblist] == 0} {
        vfs::filesystem posixerror $::vfs::posix(ENOENT)
    }
    array set sb [lindex $sblist 0]

    if {![info exists sb(mode)] && [info exists sb(attrib)]} {
        #define FILE_ATTRIBUTE_READONLY       0x0001
        #define FILE_ATTRIBUTE_HIDDEN         0x0002
        #define FILE_ATTRIBUTE_SYSTEM         0x0004
        #define FILE_ATTRIBUTE_DIRECTORY      0x0010
        #define FILE_ATTRIBUTE_ARCHIVE        0x0020
        #define FILE_ATTRIBUTE_DEVICE         0x0040
        #define FILE_ATTRIBUTE_NORMAL         0x0080
        #define FILE_ATTRIBUTE_TEMPORARY      0x0100
        #define FILE_ATTRIBUTE_SPARSE_FILE    0x0200
        #define FILE_ATTRIBUTE_REPARSE_POINT  0x0400
        #define FILE_ATTRIBUTE_COMPRESSED     0x0800
        #define FILE_ATTRIBUTE_OFFLINE        0x1000
        #define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED 0x2000
        #define FILE_ATTRIBUTE_ENCRYPTED      0x4000
        #define _S_IFMT   0170000 0xf000      /* file type mask */
        #define _S_IFDIR  0040000 0x4000      /* directory */
        #define _S_IFCHR  0020000 0x2000      /* character special */
        #define _S_IFIFO  0010000 0x1000      /* pipe */
        #define _S_IFREG  0100000 0x8000      /* regular */
        #define _S_IREAD  0000400 0x0100      /* read permission, owner */
        #define _S_IWRITE 0000200 0x0080      /* write permission, owner */
        #define _S_IEXEC  0000100 0x0040      /* execute/search permission, owner */
        set isdir [expr {$sb(attrib) & 0x0010}]
        set isreadonly [expr {$sb(attrib) & 0x0001}]
        set sb(mode) [expr {$isdir ? 0x4000 : 0x8000}]
        set sb(mode) [expr {$sb(mode) | ($isreadonly && !$isdir ? 0x16d : 0x1ff)}]
    }
    if {![info exists sb(mode)]} {
        set sb(mode) [expr {[info exists sb(isdir)] && $sb(isdir) ? 0x41ff : 0x816d}]
    }
    if {![info exists sb(type)] && [info exists sb(isdir)]} {
        set sb(type) [expr {$sb(isdir) ? "directory" : "file"}]
    }
    if {![info exists sb(type)]} {
        set sb(type) [expr {$sb(mode) & 0x4000 ? "directory" : "file"}]
    }
    if {![info exists sb(atime)] && [info exists sb(mtime)]} {
        set sb(atime) $sb(mtime)
    }
    if {![info exists sb(ctime)] && [info exists sb(mtime)]} {
        set sb(ctime) $sb(mtime)
    }
    #::vfs::log [list res [array get sb]]
    array get sb
}

proc vfs::sevenzip::access {zipfd name mode} {
    #::vfs::log "zip-access $name $mode"
    if {$mode & 2} {
        vfs::filesystem posixerror $::vfs::posix(EROFS)
    }
    # Readable, Exists and Executable are treated as 'exists'
    # Could we get more information from the archive?
    if {[llength [$zipfd list -exact $name]]} {
        return 1
    } else {
        error "No such file"
    }

}

proc vfs::sevenzip::open {zipfd name mode permissions} {
    #::vfs::log "open $name $mode $permissions"
    # return a list of two elements:
    # 1. first element is the Tcl channel name which has been opened
    # 2. second element (optional) is a command to evaluate when
    #    the channel is closed.

    switch -- $mode {
        "" -
        "r" {
            if {[llength [$zipfd list -exact $name]] == 0} {
                vfs::filesystem posixerror $::vfs::posix(ENOENT)
            }
            if {[llength [$zipfd list -type f -exact $name]] == 0} {
                vfs::filesystem posixerror $::vfs::posix(EISDIR)
            }
            set nfd [vfs::memchan]
            fconfigure $nfd -translation binary
            $zipfd extract -c $name $nfd
            fconfigure $nfd -translation auto
            seek $nfd 0

            return [list $nfd]
        }
        default {
            vfs::filesystem posixerror $::vfs::posix(EROFS)
        }
    }
}

proc vfs::sevenzip::createdirectory {zipfd name} {
    #::vfs::log "createdirectory $name"
    vfs::filesystem posixerror $::vfs::posix(EROFS)
}

proc vfs::sevenzip::removedirectory {zipfd name recursive} {
    #::vfs::log "removedirectory $name"
    vfs::filesystem posixerror $::vfs::posix(EROFS)
}

proc vfs::sevenzip::deletefile {zipfd name} {
    #::vfs::log "deletefile $name"
    vfs::filesystem posixerror $::vfs::posix(EROFS)
}

proc vfs::sevenzip::fileattributes {zipfd name args} {
    #::vfs::log "fileattributes $args"
    switch -- [llength $args] {
        0 {
            # list strings
            return [list]
        }
        1 {
            # get value
            set index [lindex $args 0]
            return ""
        }
        2 {
            # set value
            set index [lindex $args 0]
            set val [lindex $args 1]
            vfs::filesystem posixerror $::vfs::posix(EROFS)
        }
    }
}

proc vfs::sevenzip::utime {fd path actime mtime} {
    vfs::filesystem posixerror $::vfs::posix(EROFS)
}
