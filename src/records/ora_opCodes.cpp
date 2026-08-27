#include "ora_opCodes.h"
#include "../coral_show.h"

#include <iostream>
#include <string>
#include <unordered_map>

namespace ora {

    struct RawOp {
        int layer;
        int code;
        const char *mnemonic;
        const char *desc;
    };

    constexpr RawOp rawOps[] = {

        // Layer 1: Transaction Control
        {1, 0, "KCOCOTCT", "Transaction Control"},
        {1, 1, "KTZFMT", "KTZ Format block"},
        {1, 2, "KTZRDH", "Transaction Z Redo Data Header"},
        {1, 3, "KTZARC", "KTZ Allocate Record Callback"},
        {1, 4, "KTZREP", "KTZ Replace record value"},
        {1, 5, "KTZURP", "KTZ Undo for Replace"},

        // Layer 2: Transaction Read
        {2, 0, "KCOCOTRD", "Transaction Read"},

        // Layer 3: Transaction Update
        {3, 0, "KCOCOTUP", "Transaction Update"},

        // Layer 4: Transaction Block (KTB)
        {4, 0, "KCOCOTBK", "Transaction Block"},
        {4, 1, "KTBOPCLN", "Block cleanout opcode"},
        {4, 2, "KTBPHCLN", "Physical cleanout opcode"},
        {4, 3, "KTBSARC", "Single array change"},
        {4, 4, "KTBMARC", "Multiple changes to an array"},
        {4, 5, "KTBOPFMB", "Format block"},
        {4, 6, "KTBOPBCC", "Commit-time block cleanout opcode"},
        {4, 7, "KTBOPCLNL", "ITL cleanout callback"},
        {4, 8, "KTBOPBCCL", "Transaction Block Redo Block Commit Cleanout"},

        // Layer 5: Transaction Undo (KTU)
        {5, 0, "KCOCOTUN", "Transaction Undo"},
        {5, 1, "KTURDB", "Undo block or undo record update"},
        {5, 2, "KTURDH", "Update rollback segment header"},
        {5, 3, "KTURBG", "Rollout a transaction begin"},
        {5, 4, "KTURCM", "Commit transaction (transaction table update) - no undo record"},
        {5, 5, "KTUFMT", "Create rollback segment (format) - no undo record"},
        {5, 6, "KTUIRB", "Rollback record index in an undo block"},
        {5, 7, "KTUUBG", "Begin transaction (transaction table update)"},
        {5, 8, "KTURMR", "Mark transaction as dead"},
        {5, 9, "KTUUAE", "Undo routine to rollback the extent of a rollback segment"},
        {5, 10, "KTUREH", "Redo to perform the rollback of extent of rollback segment to the segment header"},
        {5, 11, "KTUBRB", "Rollback DBA in transaction table entry"},
        {5, 12, "KTURST", "Change transaction state (in transaction table entry)"},
        {5, 13, "KTURCT", "Convert rollback segment format (V6 -> V7)"},
        {5, 14, "KTURUC", "Change extent allocation parameters in a rollback segment"},
        {5, 15, "KTURCTS", "Undo Redo Convert transaction table"},
        {5, 16, "KTURCTU", "KTU Redo for Convert to Unlimited extents format"},
        {5, 17, "KTURCTM", "KTU Redo Convert for extent Move in extent map in unlimited format to segment header"},
        {5, 18, "KTURPX", "Transaction Undo segment Redo set Parent Xid"},
        {5, 19, "KTUTSL", "Transaction start audit log record"},
        {5, 20, "KTUTSC", "Transaction continue audit log record"},
        {5, 21, "KTURCVD", "Transaction Control Redo Convert undo seg Down to 8.0 format"},
        {5, 22, "KTURPHC", "Transaction Redo Physical Changes"},
        {5, 23, "KTURDBR", "Disable Block level Recovery"},
        {5, 24, "KTURLGU", "Kernel Transaction Undo Relog Change"},
        {5, 25, "KTURJT", "Join sub Transaction"},
        {5, 26, "KTUUST", "Undo Stopper undo callback"},
        {5, 27, "KTUSMFMT", "Transaction Control System Managed us Format"},
        {5, 28, "KTUUNTP", "Undo Need To Propagate"},
        {5, 29, "KTUBDB", "Big undo"},
        {5, 30, "KTURCDTS", "Change ondisk state for a distributed transaction"},
        {5, 31, "KTUFATTRC", "Flashback Archive Txn Table Redo Callback"},
        {5, 32, "KTUFATTRS", "Flashback Archive Txn Table Redo Set"},
        {5, 33, "KTUCHNF", "Change notification commit marker"},
        {5, 34, "KTUQCNTTRC", "NTP bit for change notification"},
        {5, 35, "KTUFACTTRS", "Flashback Archive Collect Txn Table Redo Set"},

        // Layer 6: Control File & Tablespace Ops
        {6, 0, "KCOCODCF", "Control File"},
        {6, 1, "TBSCRDF", "Tablespace Remove DataFile"},
        {6, 2, "TBSCADF", "Tablespace Add DataFile"},
        {6, 3, "TBSCOFL", "Tablespace Offline"},
        {6, 4, "TBSCONL", "Tablespace Online"},
        {6, 5, "TBSCRDW", "Tablespace Read-Write"},
        {6, 6, "TBSCRDO", "Tablespace Read-Only"},
        {6, 7, "TBSCRTS", "Tablespace Remove Tablespace"},
        {6, 8, "TBSCATS", "Tablespace Add Tablespace"},
        {6, 9, "TBSCUTP", "Tablespace Undo TSPITR"},
        {6, 10, "TBSCUCV", "Tablespace undo plugged datafile convert"},
        {6, 11, "TBSCREN", "Tablespace Undo Rename"},

        // Layer 10: Index Operations (KDI)
        {10, 0, "KCOCODIX", "INDEX"},
        {10, 1, "KDICPDO", "Load index block (Loader with direct mode)"},
        {10, 2, "KDICLIN", "Insert leaf row"},
        {10, 3, "KDICLPU", "Purge leaf row"},
        {10, 4, "KDICLDE", "Mark leaf row deleted"},
        {10, 5, "KDICLRE", "Restore leaf row (clear leaf delete flags)"},
        {10, 6, "KDICLOK", "Lock index block"},
        {10, 7, "KDICULO", "Unlock index block"},
        {10, 8, "KDICLNE", "Initialize new leaf block"},
        {10, 9, "KDICAIR", "Apply ITL Redo"},
        {10, 10, "KDICLNX", "Set leaf block next link"},
        {10, 11, "KDICLPR", "Set leaf block previous link"},
        {10, 12, "KDICRSP", "Init root block after split"},
        {10, 13, "KDICLEM", "Make leaf block empty"},
        {10, 14, "KDICIMA", "Restore block before image"},
        {10, 15, "KDICBIN", "Branch block row insert"},
        {10, 16, "KDICBPU", "Branch block row purge"},
        {10, 17, "KDICBNE", "Initialize new branch block"},
        {10, 18, "KDICLUP", "Update keydata in row"},
        {10, 19, "KDICLCL", "Clear row's split flag"},
        {10, 20, "KDICLSE", "Set row's split flag"},
        {10, 21, "KDICUGE", "General undo above the cache (undo)"},
        {10, 22, "KDICULK", "Undo operation on leaf key above the cache (undo)"},
        {10, 23, "KDICREB", "Restore block to b-tree"},
        {10, 24, "KDICSIT", "Shrink ITL (transaction entries)"},
        {10, 25, "KDICFRB", "Format root block redo"},
        {10, 26, "KDICUFB", "Undo of format root block (undo)"},
        {10, 27, "KDICUFR", "Redo for undo of format root block"},
        {10, 28, "KDICUMG", "Undo for migrating block"},
        {10, 29, "KDICMG", "Redo for migrating block"},
        {10, 30, "KDICLNU", "IOT leaf block nonkey update"},
        {10, 31, "KDICDLR", "Direct load root redo"},
        {10, 32, "KDICCOM", "Combine operation for insert and restore rows"},
        {10, 33, "KDICTIX", "Temp index redo apply"},
        {10, 34, "KDICFRE", "Remove block from b-tree and empty block"},
        {10, 35, "KDICLCU", "Leaf cleanup operation"},
        {10, 36, "KDICLMN", "Supplemental logging"},
        {10, 37, "KDICULN", "Undo of non-key updates"},
        {10, 38, "KDICICU", "Logical non-key update"},
        {10, 39, "KDICBUR", "Branch update range"},
        {10, 40, "KDICBDU", "Branch DBA update"},

        // Layer 11: Row Access Operations (KDO)
        {11, 0, "KCOCODRW", "Row Access"},
        {11, 1, "KDOIUR", "Interpret Undo Record (Undo)"},
        {11, 2, "KDOIRP", "Insert Row Piece"},
        {11, 3, "KDODRP", "Drop Row Piece"},
        {11, 4, "KDOLKR", "Lock Row Piece"},
        {11, 5, "KDOURP", "Update Row Piece"},
        {11, 6, "KDOORP", "Overwrite Row Piece"},
        {11, 7, "KDOMFC", "Manipulate First Column (add or delete the 1st column)"},
        {11, 8, "KDOCFA", "Change Forwarding address"},
        {11, 9, "KDOCKI", "Change the Cluster Key Index"},
        {11, 10, "KDOSKL", "Set Key Links - Change forward and backward key links on a cluster key"},
        {11, 11, "KDOQMI", "Quick Multi-Insert (e.g. insert as select)"},
        {11, 12, "KDOQMD", "Quick Multi-Delete"},
        {11, 13, "KDOTBF", "Toggle Block Header flags"},
        {11, 14, "KDODSC", "Direct space cleanout"},
        {11, 15, "KDOMBC", "Multi-block cleanout"},
        {11, 16, "KDOLMN", "LogMiner support RM for rowpiece with only logminer columns"},
        {11, 17, "KDOLLB", "LogMiner support RM for LOB id key information"},
        {11, 18, "KDOLBE", "LogMiner support RM for LOB operation errors"},
        {11, 19, "KDOURA", "LogMiner support - array updates"},
        {11, 20, "KDOSHK", "LogMiner support - shrink"},
        {11, 21, "KDOURP2", "LogMiner support - update row piece 2"},
        {11, 22, "KDOCMP", "LogMiner support - compression"},
        {11, 23, "KDODCU", "LogMiner support - direct commit undo"},
        {11, 24, "KDOMRK", "LogMiner support - marker"},
        {11, 25, "KDOAIR", "LogMiner support - apply ITL redo"},

        // Layer 12: Cluster Operations
        {12, 0, "KCOCODCL", "Cluster"},

        // Layer 13: Transaction Segment Operations (KTS)
        {13, 0, "KCOCOTSG", "Transaction Segment"},
        {13, 1, "KTSDSF", "Data Segment Format"},
        {13, 2, "KTSFFB", "Format free list block"},
        {13, 3, "KTSRCTU", "Redo for convert to unlimited extents format"},
        {13, 4, "KTSRFSH", "Fix segment header by moving its extent to ext 0"},
        {13, 5, "KTSFRBFMT", "Format data block"},
        {13, 6, "KTSFRBLNK", "Set link value on block"},
        {13, 7, "KTSFRGRP", "Freelist related fgroup/segheader redo"},
        {13, 8, "KTSFUGRP", "Freelist related fgroup/segheader undo"},
        {13, 9, "KTSFUNLK", "Undo for linking block to xnt freelist"},
        {13, 10, "KTSBSFO", "BITMAP - Format segment header"},
        {13, 11, "KTSBBFO", "BITMAP - Format bitmap block"},
        {13, 12, "KTSBIFO", "BITMAP - Format bitmap index block"},
        {13, 13, "KTSBBREDO", "BITMAP - Redo for BMB"},
        {13, 14, "KTSBBUNDO", "BITMAP - Undo for BMB"},
        {13, 15, "KTSBIREDO", "BITMAP - Redo for index map"},
        {13, 16, "KTSBIUNDO", "BITMAP - Undo for index map"},
        {13, 17, "KTSPHFO", "Bitmap Seg - Format segment Header"},
        {13, 18, "KTSPFFO", "Bitmap Seg - Format 1-st level bitmap block"},
        {13, 19, "KTSPSFO", "Bitmap Seg - Format 2-nd level bitmap block"},
        {13, 20, "KTSPTFO", "Bitmap Seg - Format 3-rd level bitmap block"},
        {13, 21, "KTSPBFO", "Bitmap Seg - Format data block"},
        {13, 22, "KTSPFREDO", "Bitmap Seg - Redo for L1 BMB"},
        {13, 23, "KTSPFUNDO", "Bitmap Seg - Undo for L1 BMB"},
        {13, 24, "KTSPSREDO", "Bitmap Seg - Redo for L2 BMB"},
        {13, 25, "KTSPSUNDO", "Bitmap Seg - Undo for L2 BMB"},
        {13, 26, "KTSPTREDO", "Bitmap Seg - Redo for L3 BMB"},
        {13, 27, "KTSPTUNDO", "Bitmap Seg - Undo for L3 BMB"},
        {13, 28, "KTSPHREDO", "Bitmap Seg - Redo for pagetable segment header block"},
        {13, 29, "KTSPHUNDO", "Bitmap Seg - Undo for pagetable segment header block"},
        {13, 30, "KTSPLBFFO", "Bitmap Seg - Format L1 BMB for LOB segments"},
        {13, 31, "KTSKFREDO", "Bitmap Seg - Shrink redo for L1"},
        {13, 32, "KTSKHREDO", "Bitmap Seg - Shrink redo for segment header"},
        {13, 33, "KTSKEREDO", "Bitmap Seg - Shrink redo for extent map blk"},
        {13, 34, "KTSKHUNDO", "Bitmap Seg - Shrink undo for segment header"},
        {13, 35, "KTSKFUNDO", "Bitmap Seg - Shrink undo for L1"},
        {13, 36, "KTSKSREDO", "Bitmap Seg - Shrink redo related"},
        {13, 37, "KTSKSUNDO", "Bitmap Seg - Shrink undo related"},
        {13, 38, "KTSKTREDO", "Bitmap Seg - Shrink redo target"},
        {13, 39, "KTSKTUNDO", "Bitmap Seg - Shrink undo target"},
        {13, 40, "KTSKEUNDO", "Bitmap Seg - Shrink undo for extent map blk"},
        {13, 41, "KTSLEFREDO", "NGLOB format opcode Extent Header"},
        {13, 42, "KTSLPFREDO", "NGLOB format opcode Persistent Undo"},
        {13, 43, "KTSLHFREDO", "NGLOB format opcode Hash bucket"},
        {13, 44, "KTSLFFREDO", "NGLOB format opcode Free Space"},
        {13, 45, "KTSLSFREDO", "NGLOB format opcode Segment Header"},
        {13, 46, "KTSLBFREDO", "NGLOB format opcode data block"},
        {13, 47, "KTSLEUREDO", "NGLOB block update Extent Header redo"},
        {13, 48, "KTSLEUUNDO", "NGLOB block update Extent Header undo"},
        {13, 49, "KTSLHUREDO", "NGLOB block update Hash Bucket redo"},
        {13, 50, "KTSLHUUNDO", "NGLOB block update Hash Bucket undo"},
        {13, 51, "KTSLFUREDO", "NGLOB block update Free Space redo"},
        {13, 52, "KTSLFUUNDO", "NGLOB block update Free Space undo"},
        {13, 53, "KTSLPUREDO", "NGLOB block update Persistent Undo redo"},
        {13, 54, "KTSLPUUNDO", "NGLOB block update Persistent Undo undo"},
        {13, 55, "KTSLSUREDO", "NGLOB block update Segment Header redo"},
        {13, 56, "KTSLSUUNDO", "NGLOB block update Segment Header undo"},

        // Layer 14: Transaction Extent Operations (KTE)
        {14, 0, "KCOCOTEX", "Transaction Extent"},
        {14, 1, "KTECUSH", "Unlock Segment Header"},
        {14, 2, "KTECRLK", "Redo set extent map disk Lock"},
        {14, 3, "KTEFRCU", "Redo for conversion to unlimited format"},
        {14, 4, "KTEOPEMREDO", "Extent operation redo"},
        {14, 5, "KTEOPEUNDO", "Extent operation undo"},
        {14, 6, "KTEOPEFREDO", "Extent map format redo"},
        {14, 7, "KTECNV", "Extent map convert redo"},
        {14, 8, "KTEOPUTRN", "Undo for truncate ops, flush the object"},
        {14, 9, "KTEFUCTL", "Undo for reformat of a ctl block"},
        {14, 10, "KTEFRCTL", "Redo to facilitate above undo"},
        {14, 11, "KTECRCLN", "Redo to clean XIDs in seghdr/fgb"},
        {14, 12, "KTEOPRPECT", "SMU Retention Redo to propagate extent commit time"},

        // Layer 15: Table Space Save Undo (KTT)
        {15, 0, "KCOCOTTS", "Table Space"},
        {15, 1, "KTTFSU", "Format save undo header"},
        {15, 2, "KTTSUN", "Add save undo record"},
        {15, 3, "KTTNBK", "Move to next block"},
        {15, 4, "KTTNAS", "Point to next save undo record"},
        {15, 5, "KTTUSB", "Update saveundo blk during save undo application"},

        // Layer 16: Row Cache
        {16, 0, "KCOCOQRC", "Row Cache"},

        // Layer 17: Recovery Operations (KCV)
        {17, 0, "KCOCORCV", "Recovery (REDO)"},
        {17, 1, "KCVOPEHB", "End Hot Backup - clears hot backup in-progress flags"},
        {17, 2, "KCVOPENT", "Enable Thread - signals thread enabled"},
        {17, 3, "KCVOPCRM", "Crash Recovery Marker"},
        {17, 4, "KCVOPRSZ", "Resizeable datafiles"},
        {17, 5, "KCVOPONL", "Tablespace Online"},
        {17, 6, "KCVOPOFF", "Tablespace Offline"},
        {17, 7, "KCVOPRDW", "Tablespace Read Write"},
        {17, 8, "KCVOPRDO", "Tablespace Read Only"},
        {17, 9, "KCVOPADD", "Adding datafiles to database"},
        {17, 10, "KCVOPDRP", "Tablespace Drop"},
        {17, 11, "KCVOPTPR", "Tablespace PITR"},
        {17, 12, "KCVOPPLG_PRE10GR2", "Plugging datafiles to database (Pre-10gR2)"},
        {17, 13, "KCVOPCNV", "Convert plugged in datafiles"},
        {17, 14, "KCVOPADF_PRE10GR2", "Adding datafiles to database (Pre-10gR2)"},
        {17, 15, "KCVOPHBR", "Heart-beat redo"},
        {17, 16, "KCVOPTRN", "Tablespace rename"},
        {17, 17, "KCVOPENT_10GR2", "Enable Thread (10gR2)"},
        {17, 18, "KCVOPONL_10GR2", "Tablespace Online (10gR2)"},
        {17, 19, "KCVOPOFF_10GR2", "Tablespace Offline (10gR2)"},
        {17, 20, "KCVOPRDW_10GR2", "Tablespace Read Write (10gR2)"},
        {17, 21, "KCVOPRDO_10GR2", "Tablespace Read Only (10gR2)"},
        {17, 22, "KCVOPPLG_10GR2", "Plugging datafiles to db (10gR2)"},
        {17, 23, "KCVOPADF_10GR2", "Adding datafiles to database (10gR2)"},
        {17, 24, "KCVOPCNV_10GR2", "Convert plugged in datafiles (10gR2)"},
        {17, 25, "KCVOPTPR_10GR2", "Tablespace PITR (10gR2)"},
        {17, 26, "KCVOPFDP", "File drop in tablespace"},
        {17, 27, "KCVOPIEN", "Internal thread enable"},
        {17, 28, "KCVOPMFL", "Readable standby metadata flush"},
        {17, 29, "KCVOPDBK", "Database key creation (11g+)"},
        {17, 30, "KCVOPADF", "Adding datafiles to database"},
        {17, 31, "KCVOPPLG", "Plugging datafiles to db"},
        {17, 32, "KCVOPSPHUPD", "Modifying space header info"},
        {17, 33, "KCVOPTMR", "TSE Masterkey Rekey"},

        // Layer 18: Hot Backup Log Blocks
        {18, 0, "KCOCOHLB", "Hot Backup Log Blocks"},
        {18, 1, "KCBKCOLB", "Log block image"},
        {18, 2, "KCBKCORV", "Recovery testing"},
        {18, 3, "KCBKCOREU", "Object/Range reuse"},

        // Layer 19: Direct Loader Log Blocks
        {19, 0, "KCOCODLB", "Direct Loader Log Blocks"},
        {19, 1, "KCBLCOLB", "Direct block logging"},
        {19, 2, "KCBLCOIR", "Invalidate range"},
        {19, 3, "KCBLCRLB", "Direct block relogging"},
        {19, 4, "KCBLCRIR", "Invalidate range relogging"},

        // Layer 20: Compatibility Segment Operations
        {20, 0, "KCOCOKCK", "Compatibility Segment operations"},
        {20, 1, "KCKFCS", "Format compatibility segment"},
        {20, 2, "KCKUCS", "Update compatibility segment"},
        {20, 3, "KCKURD", "Update Root DBA in controlfile and file header 1"},
        {20, 4, "KCK_INV_SQL_SIG", "Set bit in SQL Tuning Existence Bit Vector"},
        {20, 5, "KCK_INV_SQL_SIG", "Invalidate SQL Statement by Signature"},
        {20, 6, "KCK_UNAUTH_CUR", "Unauthorize cursors after sys privilege revoke"},

        // Layer 21: LOB Segment Operations
        {21, 0, "KCOCOLFS", "LOB segment operations"},
        {21, 1, "KDLOPWRI", "Write data into ILOB data block"},

        // Layer 22: Tablespace Bitmapped File Operations
        {22, 0, "KCOCOTBF", "Tablespace bitmapped file operations"},
        {22, 1, "KTFBHFO", "Format space header"},
        {22, 2, "KTFBHREDO", "Space header generic redo"},
        {22, 3, "KTFBHUNDO", "Space header undo"},
        {22, 4, "KTFBBFO", "Space bitmap block format"},
        {22, 5, "KTFBBREDO", "Bitmap block generic redo"},

        // Layer 23: Write Behind Logging Operations
        {23, 0, "KCOCOLWR", "Write behind logging of blocks"},
        {23, 1, "KCBBLWR", "Dummy block written callback"},
        {23, 2, "KCBBLRD", "Log reads"},
        {23, 3, "KCBBLDWR", "Log DirectWrites"},

        // Layer 24: LogMiner Operations
        {24, 0, "KCOCOKRV", "Logminer related (DDL or OBJV# redo)"},
        {24, 1, "KRVDDL", "Common portion of DDL"},
        {24, 2, "KRVDLR", "Direct load redo"},
        {24, 3, "KRVLOB", "LOB related info"},
        {24, 4, "KRVMISC", "Miscellaneous info"},
        {24, 5, "KRVUSER", "User info"},
        {24, 6, "KRVDLR10", "Direct load redo 10g"},
        {24, 7, "KRVUOP", "LogMiner undo opcode"},
        {24, 8, "KRVXML", "XML redo - doc or diff - opcode"},
        {24, 9, "KRVPLSQL", "PL/SQL redo"},
        {24, 10, "KRVURU", "Uniform Redo Unchained"},
        {24, 11, "KRVCMT", "Transaction commit marker"},
        {24, 12, "KRVCFF", "Supplemental logging marker"},

        // Layer 25: Queue Operations
        {25, 0, "KCOCOQUE", "Queue Related"},
        {25, 1, "KDQSUN", "Undo"},
        {25, 2, "KDQSIN", "Init"},
        {25, 3, "KDQSEN", "Enqueue"},
        {25, 4, "KDQSUP", "Update"},
        {25, 5, "KDQSDL", "Delete"},
        {25, 6, "KDQSLK", "Lock"},
        {25, 7, "KDQSMM", "Min/Max"},

        // Layer 26: Local LOB Operations
        {26, 0, "KCOCOLOB", "Local LOB Related"},
        {26, 1, "KDLIRUNDO", "Generic LOB undo"},
        {26, 2, "KDLIRREDO", "Generic LOB redo"},
        {26, 3, "KDLIRFRMT", "LOB block format redo"},
        {26, 4, "KDLIRINVL", "LOB invalidation redo"},
        {26, 5, "KDLIRLOAD", "LOB cache-load redo"},
        {26, 6, "KDLIRBIMG", "Direct LOB direct-load redo"},
        {26, 7, "KDLIRCALI", "Dummy calibration redo"},

        // Layer 27: Block Change Tracking
        {27, 0, "KCOCOBCT", "Block Change Tracking"},
        {27, 1, "KRCPBSW", "Opcode for bitmap switch"}
    };

    static const std::unordered_map<int, std::unordered_map<int, std::string> > codeMap
            = [] {
                std::unordered_map<int, std::string> headers;

                for (const auto &op: rawOps) {
                    if (op.code == 0)
                        headers[op.layer] = op.desc;
                }

                std::unordered_map<int, std::unordered_map<int, std::string> > out;

                for (const auto &op: rawOps) {
                    if (op.code != 0) {
                        out[op.layer][op.code] =
                                fmt::format("{}.{} {} {}: {}",
                                            op.layer, op.code, op.mnemonic, headers[op.layer],
                                            op.desc);
                    }
                }

                return out;
            }();

    std::string opCode_string(const uint8_t layer, const uint8_t code) {
        const auto layerIt = codeMap.find(layer);
        if (layerIt == codeMap.end()) {
            return fmt::format("{}.{} Unknown layer", layer, code);
        }

        const auto codeIt = layerIt->second.find(code);
        if (codeIt == layerIt->second.end()) {
            return fmt::format("{}.{} Unknown code", layer, code);
        }
        return codeIt->second;
    }

    // ================================================================================
    struct RawCType {
        int layer;
        int code;
        int ctype;
        const char *desc;
    };

    constexpr RawCType rawCTypes[] = {
        // =========================================================================
        // LAYER 4: KTB (Kernel Transaction Block)
        // =========================================================================
        {4, 1, 0, "Normal Block Cleanout (일반 블록 클린아웃)"},
        {4, 1, 1, "Delayed Block Cleanout (지연 블록 클린아웃)"},
        {4, 1, 2, "Commit Cleanout (커밋 시점 블록 클린아웃)"},

        // =========================================================================
        // LAYER 5: KTU (Kernel Transaction Undo) - 트랜잭션 및 언두 관리
        // =========================================================================
        // --- OP 5.1 (KTURDB): Undo Block or Undo Segment Header ---
        {5, 1, 0, "KTU_UB_NORMAL (일반 언두 블록/레코드 업데이트)"},
        {5, 1, 1, "KTU_UB_ROLLBACK (트랜잭션 롤백에 의한 언두 적용)"},
        {5, 1, 2, "KTU_UB_CR (Consistent Read/일기 일관성 생성을 위한 언두)"},
        {5, 1, 3, "KTU_UB_PARTIAL_ROLLBACK (부분 롤백/SAVEPOINT 롤백)"},
        {5, 1, 4, "KTU_UB_PARALLEL_ROLLBACK (병렬 복구/Fast-Start Parallel Rollback)"},

        // --- OP 5.2 (KTURDH): Update Rollback Segment Header ---
        {5, 2, 0, "KTU_URDH_NORMAL (언두 세그먼트 헤더 상태 업데이트)"},
        {5, 2, 1, "KTU_RDH_SHRINK (언두 세그먼트 수축/Shrink 조작)"},
        {5, 2, 2, "KTU_RDH_WRAP (언두 세그먼트 Extent Wrap/새 익스텐트 이동)"},

        // --- OP 5.3 (KTURBG): Rollout a Transaction Begin ---
        {5, 3, 0, "KTU_RBG_NORMAL (롤아웃 트랜잭션 시작)"},

        // --- OP 5.4 (KTURCM): Commit Transaction ---
        {5, 4, 0, "KTU_RCM_NORMAL (표준 트랜잭션 커밋)"},
        {5, 4, 1, "KTU_RCM_READ_ONLY (변경사항 없는 Read-Only 트랜잭션 종료)"},
        {5, 4, 2, "KTU_RCM_DISTRIBUTED (분산/2PC 2-Phase Commit 완료)"},
        {5, 4, 3, "KTU_RCM_DELAYED_CLEANOUT (지연 블록 클린아웃 처리 커밋)"},
        {5, 4, 4, "KTU_RCM_FLASHBACK_MARKER (Flashback Archive/변경 알림 커밋 마커)"},

        // --- OP 5.5 (KTUFMT): Create Rollback Segment (Format) ---
        {5, 5, 0, "KTU_FMT_NORMAL (언두 블록/세그먼트 초기화 포맷)"},

        // --- OP 5.6 (KTUIRB): Rollback Record Index in Undo Block ---
        {5, 6, 0, "KTU_IRB_NORMAL (언두 블록 내 레코드 인덱스 롤백)"},

        // --- OP 5.7 (KTUUBG): Begin Transaction (Transaction Table Update) ---
        {5, 7, 0, "KTU_UBG_NORMAL (일반 트랜잭션 시작 - Tx 슬롯 할당)"},
        {5, 7, 1, "KTU_UBG_AUTONOMOUS (자율 트랜잭션 Autonomous Transaction 시작)"},
        {5, 7, 2, "KTU_UBG_DISTRIBUTED (분산 트랜잭션 시작)"},

        // --- OP 5.8 (KTURMR): Mark Transaction as Dead ---
        {5, 8, 0, "KTU_RMR_NORMAL (비정상 종료 트랜잭션 PMON/Dead 마킹)"},

        // --- OP 5.11 (KTUBRB): Rollback DBA in Transaction Table Entry ---
        {5, 11, 0, "KTU_BRB_NORMAL (트랜잭션 테이블 엔트리의 Rollback DBA 변경)"},

        // --- OP 5.12 (KTURST): Change Transaction State ---
        {5, 12, 0, "KTU_RST_NORMAL (트랜잭션 상태 전환 - Active/Prepared/Committed)"},

        // --- OP 5.19 (KTUTSL): Transaction Start Audit Log Record ---
        {5, 19, 0, "KTU_TSL_NORMAL (트랜잭션 시작 감사 로그 기록)"},
        {5, 19, 6, "KTU_TSL_MARKER (트랜잭션 시작 감사 마커 레코드)"},

        // --- OP 5.20 (KTUTSC): Transaction Continue Audit Log Record ---
        {5, 20, 0, "KTU_TSC_NORMAL (트랜잭션 지속 감사 로그 기록)"},
        {5, 20, 6, "KTU_TSC_MARKER (트랜잭션 지속 감사 마커 레코드)"},

        // --- OP 5.23 (KTURDBR): Disable Block Level Recovery ---
        {5, 23, 0, "KTU_DBR_NORMAL (일반 블록 레벨 복구 비활성화)"},

        // --- OP 5.24 (KTURLGU): Relog Change ---
        {5, 24, 0, "KTU_LGU_NORMAL (언두 재기록/Relog 변경)"},

        // --- OP 5.26 (KTUUST): Undo Stopper Undo Callback ---
        {5, 26, 0, "KTU_UST_NORMAL (언두 롤백 중단 및 콜백 처리)"},

        // --- OP 5.27 (KTUSMFMT): System Managed Undo Format ---
        {5, 27, 0, "KTU_SMFMT_NORMAL (자동 언두 관리 AUM 세그먼트 포맷)"},

        // --- OP 5.29 (KTUBDB): Big Undo ---
        {5, 29, 0, "KTU_BDB_NORMAL (대형 트랜잭션 Undo 전환 마커)"},

        // --- OP 5.30 (KTURCDTS): Change On-disk State for Distributed Tx ---
        {5, 30, 0, "KTU_RCDTS_NORMAL (분산 트랜잭션 온디스크 상태 변경)"},

        // --- OP 5.33 (KTUCHNF): Change Notification Commit Marker ---
        {5, 33, 0, "KTU_CHNF_NORMAL (DB Change Notification 커밋 마커 기록)"},

        // =========================================================================
        // LAYER 10: KDI (Kernel Data Index) - 인덱스 조작
        // =========================================================================
        {10, 2, 0, "Normal Leaf Row Insert (일반 인덱스 엔트리 삽입)"},
        {10, 2, 1, "Direct Load Leaf Insert (Direct Load 인덱스 삽입)"},
        {10, 2, 2, "Unique Key Check Insert (유니크 키 제약 검증 삽입)"},
        {10, 3, 0, "Normal Leaf Row Purge (일반 인덱스 리프 행 퍼지)"},
        {10, 4, 0, "Normal Leaf Row Delete Mark (일반 인덱스 엔트리 삭제 표시)"},
        {10, 4, 1, "Purge Leaf Row (인덱스 엔트리 영구 삭제)"},
        {10, 4, 2, "Rollback Leaf Row Delete (인덱스 엔트리 삭제 취소/롤백)"},
        {10, 5, 0, "Restore Leaf Row (인덱스 리프 행 복원 및 복구)"},
        {10, 18, 0, "Index Leaf Compression/Fast Clean (인덱스 리프 블록 압축 및 재정비)"},
        {10, 18, 2, "Index Fast Split / Direct Reorg (인덱스 고속 분할 및 재구성)"},
        {10, 35, 0, "Index Advanced Compression Redo (인덱스 고급 압축 변경)"},


        // =========================================================================
        // LAYER 11: KDO (Kernel Data Objects) - 행(Row) 수준 접근 및 조작
        // =========================================================================
        // --- OP 11.1 (KDOIUR): Interpret Undo Record (Undo) ---
        {11, 1, 0, "KDO_IUR_NORMAL (일반 언두 레코드 해석 및 적용)"},
        {11, 1, 1, "KDO_IUR_INSERT_UNDO (Insert 취소용 삭제 언두 적용)"},
        {11, 1, 2, "KDO_IUR_DELETE_UNDO (Delete 취소용 재삽입 언두 적용)"},
        {11, 1, 3, "KDO_IUR_UPDATE_UNDO (Update 이전 값 원복 언두 적용)"},

        // --- OP 11.2 (KDOIRP): Insert Row Piece ---
        {11, 2, 0, "KDO_IRP_NORMAL (일반 행 삽입)"},
        {11, 2, 1, "KDO_IRP_DIRECT (Direct Path Insert / Direct Load 삽입)"},
        {11, 2, 2, "KDO_IRP_PIECE (조각난 행 Piece/Split 삽입)"},
        {11, 2, 3, "KDO_IRP_FIRST_PIECE (Chained Row의 첫 번째 조각 삽입)"},
        {11, 2, 4, "KDO_IRP_CLUSTER (클러스터 테이블 행 삽입)"},
        {11, 2, 5, "KDO_IRP_SUPPLE_LOG (Supplemental Logging 추가 데이터 수집 삽입)"},
        {11, 2, 8, "KDO_IRP_MULTI_PIECE (다중 조각 일할 삽입)"},

        // --- OP 11.3 (KDODRP): Drop Row Piece ---
        {11, 3, 0, "KDO_DRP_NORMAL (일반 행 삭제 / Mark Delete)"},
        {11, 3, 1, "KDO_DRP_PURGE (빠른 삭제 및 공간 즉시 정제)"},
        {11, 3, 2, "KDO_DRP_CASCADE (연쇄/FK 삭제 조작)"},
        {11, 3, 4, "KDO_DRP_CLUSTER (클러스터 테이블 행 삭제)"},

        // --- OP 11.4 (KDOLKR): Lock Row Piece ---
        {11, 4, 0, "KDO_LKR_NORMAL (일반 행 수준 잠금 / Row Lock)"},
        {11, 4, 1, "KDO_LKR_SHARED (공유 행 잠금 / Cluster Key Lock)"},
        {11, 4, 2, "KDO_LKR_ROLLBACK (행 잠금 해제/롤백 조작)"},

        // --- OP 11.5 (KDOURP): Update Row Piece ---
        {11, 5, 0, "KDO_URP_NORMAL (일반 행 수정)"},
        {11, 5, 1, "KDO_URP_QUICK (Quick Update / 단일 컬럼 제자리 Fast Update)"},
        {11, 5, 2, "KDO_URP_NULL (Null 값 변환 수정)"},
        {11, 5, 3, "KDO_URP_INPLACE_RESIZE (길이 변경 제자리 수정)"},
        {11, 5, 4, "KDO_URP_OUT_OF_PLACE (비제자리/이동 수정)"},
        {11, 5, 8, "KDO_URP_MIGRATED (마이그레이션된 Row 수정)"},
        {11, 5, 16, "KDO_URP_SUPPLE_LOG (Supplemental Logging 용 Update 기록)"},

        // --- OP 11.6 (KDOORP): Overwrite Row Piece ---
        {11, 6, 0, "KDO_ORP_NORMAL (행 데이터 덮어쓰기)"},

        // --- OP 11.7 (KDOMFC): Manipulate First Column ---
        {11, 7, 0, "KDO_MFC_ADD_FIRST (첫 번째 컬럼 추가 조작)"},
        {11, 7, 1, "KDO_MFC_DEL_FIRST (첫 번째 컬럼 삭제 조작)"},

        // --- OP 11.8 (KDOCFA): Change Forwarding Address ---
        {11, 8, 0, "KDO_CFA_NORMAL (Row Migration 발생 시 포워딩 주소 변경)"},

        // --- OP 11.9 (KDOCKI): Change the Cluster Key Index ---
        {11, 9, 0, "KDO_CKI_NORMAL (클러스터 키 인덱스 변경)"},

        // --- OP 11.10 (KDOSKL): Set Key Links ---
        {11, 10, 0, "KDO_SKL_NORMAL (클러스터 키 연결 포인터 변경)"},

        // --- OP 11.11 (KDOQMI): Quick Multi-Insert ---
        {11, 11, 0, "KDO_QMI_NORMAL (대량 고속 일괄 삽입 - INSERT SELECT 등)"},
        {11, 11, 1, "KDO_QMI_DIRECT (Direct Path 고속 일괄 삽입)"},
        {11, 11, 2, "KDO_QMI_MULTI_PIECE (대량 고속 다중 조각/부분 일괄 삽입)"},

        // --- OP 11.12 (KDOQMD): Quick Multi-Delete ---
        {11, 12, 0, "KDO_QMD_NORMAL (대량 고속 일괄 삭제)"},

        // --- OP 11.13 (KDOTBF): Toggle Block Header Flags ---
        {11, 13, 0, "KDO_TBF_NORMAL (데이터 블록 헤더 플래그 변경)"},

        // --- OP 11.16 (KDOLMN): LogMiner Support RM for RowPiece ---
        {11, 16, 0, "KDO_LMN_NORMAL (LogMiner 전용 Supplemental Logging Row)"},
        {11, 16, 1, "KDO_LMN_KEY_ONLY (LogMiner 키 컬럼 전용 로깅)"},

        // --- OP 11.17 (KDOLLB): LogMiner Support for LOB ID Key ---
        {11, 17, 0, "KDO_LLB_NORMAL (LogMiner LOB ID 키 정보 로깅)"},
        {11, 17, 6, "LogMiner LOB Key Marker (LogMiner 전용 LOB 키 메타데이터 마커)"},

        // --- OP 11.19 (KDOURA): LogMiner Support - Array Updates ---
        {11, 19, 0, "KDO_URA_NORMAL (LogMiner 배열 수정 로깅)"},

        // =========================================================================
        // LAYER 13: KTS (Kernel Transaction Segment) - 세그먼트 및 공간 관리
        // =========================================================================
        {13, 1, 1, "Format Data Segment Header (데이터 세그먼트 헤더 포맷/초기화)"},
        {13, 5, 1, "Format Data Block (데이터 블록 포맷/초기화)"},
        {13, 6, 0, "Normal Link Value Set (일반 데이터 블록 프리리스트 링크 설정)"},
        {13, 7, 0, "Normal Freelist/Group Header Redo (일반 프리리스트/그룹 헤더 변경)"},
        {13, 17, 1, "Format Segment Header (세그먼트 헤더 포맷/초기화)"},
        {13, 18, 1, "Format L1 BMB (L1 비트맵 블록 포맷/초기화)"},
        {13, 19, 1, "Format L2 BMB (L2 비트맵 블록 포맷/초기화)"},
        {13, 21, 1, "Format Data Block (데이터 블록 포맷/초기화)"},
        {13, 22, 0, "Normal L1 BMB Redo (일반 L1 비트맵 블록 변경)"},
        {13, 24, 0, "Normal L2 BMB Redo (일반 L2 비트맵 블록 변경)"},
        {13, 28, 0, "Normal Segment Header Redo (일반 세그먼트 헤더 변경)"},
        {13, 49, 0, "Normal SecureFiles/Hash Bucket Redo (일반 L1/해시 버킷 변경)"},
        {13, 53, 0, "Normal Persistent Undo Redo (일반 지속성 언두 변경)"},
        {13, 57, 1, "Format SecureFiles Segment Header (SecureFiles 세그먼트 헤더 포맷/초기화)"},
        {13, 59, 1, "Format Bitmap Block (비트맵 블록 포맷/초기화)"},
        {13, 60, 0, "Normal Space Bitmap Redo (일반 공간 비트맵 변경)"},
        {13, 61, 1, "Format L1 BMB (L1 비트맵 블록 포맷/초기화)"},
        {13, 62, 0, "Normal L1 BMB Redo (일반 L1 비트맵 블록 변경)"},
        {13, 63, 1, "Format L2 BMB (L2 비트맵 블록 포맷/초기화)"},
        {13, 64, 0, "Normal L2 BMB Redo / Format L3 BMB (일반 L2 비트맵 블록 / L3 BMB 변경)"},

        // =========================================================================
        // LAYER 14: KTE (Kernel Transaction Extent) - 익스텐트 관리
        // =========================================================================
        {14, 1, 0, "Normal Extent Action (일반 익스텐트 할당/해제)"},
        {14, 1, 1, "Auto-allocate Extent (자동 익스텐트 확장)"},
        {14, 2, 0, "Normal Extent Map Lock (일반 익스텐트/언두 세그먼트 맵 디스크 락 변경)"},
        {14, 4, 0, "Normal Extent Operation Redo (일반 익스텐트 할당/해제 리두)"},

        // =========================================================================
        // LAYER 17 / 18 / 22 / 23 / 24 / 26: 기타 시스템 및 마커 작업
        // =========================================================================
        {17, 3, 6, "Crash Recovery Marker (크래시 복구 지점 마커)"},
        {17, 15, 6, "Heartbeat Redo Marker (하트비트/체크포인트 상태 마커)"},
        {17, 28, 6, "System Container/State Marker (시스템/PDB 상태 변경 복구 마커)"},
        {18, 3, 6, "Object/Range Reuse Marker (오브젝트/범위 재사용 마커)"},
        {22, 2, 0, "Normal Space Header Redo (일반 공간 헤더 블록 변경)"},
        {22, 5, 0, "Normal Bitmap Index/Space Redo (일반 비트맵 인덱스/공간 블록 변경)"},
        {22, 15, 1, "Format Bitmap File Space Block (비트맵 파일 공간 블록 초기화)"},
        {22, 16, 0, "Normal Bitmap File Space Redo (일반 비트맵 파일 공간 변경)"},
        {23, 1, 6, "Dummy Block Written Marker (더미 블록 기록 콜백 마커)"},
        {23, 3, 6, "Direct Write Logging Marker (다이렉트 라이트 로깅 마커)"},
        {24, 1, 6, "LogMiner DDL Marker (LogMiner DDL 메타데이터 마커)"},
        {24, 4, 6, "LogMiner Misc Marker (LogMiner 기타 메타데이터 마커)"},
        {24, 5, 6, "LogMiner User Info Marker (LogMiner 사용자 정보 마커)"},
        {24, 10, 6, "LogMiner Unchained Redo Marker (LogMiner 단일 리두 마커)"},
        {26, 6, 10, "Direct LOB Image Write (Direct Path LOB 데이터 블록 이미지 기록)"}
    };

    static const std::unordered_map<uint32_t, std::string> cTypeMap = [] {
        std::unordered_map<uint32_t, std::string> out;
        for (const auto &item: rawCTypes) {
            uint32_t key = static_cast<uint32_t>(item.layer) << 16 |
                           static_cast<uint32_t>(item.code) << 8 |
                           static_cast<uint32_t>(item.ctype);
            out[key] = item.desc;
        }
        return out;
    }();

    std::string cType_string(const uint8_t layer, const uint8_t code, const uint8_t ctype) {
        const uint32_t key = static_cast<uint32_t>(layer) << 16 |
                             static_cast<uint32_t>(code) << 8 |
                             static_cast<uint32_t>(ctype);

        const auto it = cTypeMap.find(key);
        if (it != cTypeMap.end()) {
            return fmt::format("    {} CTy: {} ({}.{})",  it->second, ctype, layer, code);
        }
        return fmt::format("    todo CTy: {} ({}.{})", ctype, layer, code);
    }
}
