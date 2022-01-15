export type WinCeCab000Header = {
    appName: string;
    provider: string;
    unsupported?: string[];

    minCeVersion?: {
        minCeVersionMinor: number;
        minCeVersionMajor: number;
        minCeVersionString: string;
    };
    maxCeVersion?: {
        maxCeVersionMinor: number;
        maxCeVersionMajor: number;
        maxCeVersionString: string;
    };
    minCeBuildNumber?: number;
    maxCeBuildNumber?: number;

    files: {
        fileId: number;
        directory: string;
        /** If bit is set, this file is a reference-counting shared file. It is not deleted at uninstall time unless its reference count is 0 */
        isReferenceCountingSharedFile: boolean;
        /** If bit is set, ignore file date (stored in the cabinet file) and always overwrite target (on CE device). Mutually exclusive with bit 29 */
        ignoreCabFileDate: boolean;
        /** If bit is set, do not overwrite target if target is newer. Mutually exclusive with bit 30 */
        doNotOverWriteIfTargetIsNewer: boolean;
        /** If bit is set, self-register this DLL */
        selfRegisterDll: boolean;
        /** If bit is set, do not copy this file to the target unless the target already exists. Mutually exclusive with bit 4 */
        doNotCopyUnlessTargetExists: boolean;
        /** If bit is set, do not overwrite target if it already exists. Mutually exclusive with bit 10 */
        overWriteTargetIfExists: boolean;
        /** If bit is set, do not skip this file */
        doNotSkip: boolean;
        /** If bit is set, warn the user if this file is skipped */
        warnIfSkipped: boolean;
    }[];

    registryEntries: {
        /** Registry hive */
        hive: string;
        /** Registry keys */
        keys: {
            /** Registry key */
            [key: string]: ({
                type?: "REG_DWORD" | "REG_BINARY";
                value: string;
            } | {
                type: "REG_MULTI_SZ";
                value: string[];
            })[];
        };
    }[];

};