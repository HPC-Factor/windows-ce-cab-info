import { WinCEArchitecture } from "./WinCEArchitecture";

export type unsupported = "PALM-SIZE PC" | "HPC" | "PALM PC" | "PALM PC2" | "POCKETPC" | "JUPITER";

//First, for a Handheld PC:
export const DIRECTORY_MAPPINGS_HPC: { [key: string]: string; } = {
    "%CE1%": "\\Program Files",
    "%CE2%": "\\Windows",
    "%CE3%": "\\Windows\\Desktop",
    "%CE4%": "\\Windows\\StartUp",
    "%CE5%": "\\My Documents",
    "%CE6%": "\\Program Files\\Accessories",
    "%CE7%": "\\Program Files\\Communications",
    "%CE8%": "\\Program Files\\Games",
    "%CE9%": "\\Program Files\\Pocket Outlook",
    "%CE10%": "\\Program Files\\Office",
    "%CE11%": "\\Windows\\Programs",
    "%CE12%": "\\Windows\\Programs\\Accessories",
    "%CE13%": "\\Windows\\Programs\\Communications",
    "%CE14%": "\\Windows\\Programs\\Games",
    "%CE15%": "\\Windows\\Fonts",
    "%CE16%": "\\Windows\\Recent",
    "%CE17%": "\\Windows\\Favorites"
};

export const DIRECTORY_MAPPINGS_PSPC: { [key: string]: string; } = {
    "%CE1%": "\\Program Files",
    "%CE2%": "\\Windows",
    "%CE3%": undefined,
    "%CE4%": "\\Windows\\StartUp",
    "%CE5%": "\\My Documents",
    "%CE6%": "\\Program Files\\Accessories",
    "%CE7%": "\\Program Files\\Communications",
    "%CE8%": "\\Program Files\\Games",
    "%CE9%": undefined,
    "%CE10%": undefined,
    "%CE11%": "\\Windows\\Start Menu\\Programs",
    "%CE12%": "\\Windows\\Start Menu\\Accessories",
    "%CE13%": "\\Windows\\Start Menu\\Communications",
    "%CE14%": "\\Windows\\Start Menu\\Games",
    "%CE15%": "\\Windows\\Fonts",
    "%CE16%": undefined,
    "%CE17%": "\\Windows\\Start Menu"
};

export const DIRECTORY_MAPPINGS_PPC: { [key: string]: string; } = {
    "%CE1%": "\\Program Files",
    "%CE2%": "\\Windows",
    "%CE3%": undefined,
    "%CE4%": "\\Windows\\StartUp",
    "%CE5%": "\\My Documents",
    "%CE6%": undefined,
    "%CE7%": undefined,
    "%CE8%": undefined,
    "%CE9%": undefined,
    "%CE10%": undefined,
    "%CE11%": "\\Windows\\Start Menu\\Programs",
    "%CE12%": undefined,
    "%CE13%": undefined,
    "%CE14%": "\\Windows\\Start Menu\\Games",
    "%CE15%": "\\Windows\\Fonts",
    "%CE16%": undefined,
    "%CE17%": "\\Windows\\Start Menu"
};

export type WinCeCab000Header = {
    appName: string;
    provider: string;
    architecture: WinCEArchitecture | null;
    unsupported?: string[];

    minCeVersion?: {
        major: number;
        minor: number;
        stringValue: string;
    };
    maxCeVersion?: {
        major: number;
        minor: number;
        stringValue: string;
    };
    minCeBuildNumber?: number;
    maxCeBuildNumber?: number;

    directories: {
        id: number,
        path: string;
    }[];

    files: {
        id: number;
        name: string;
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
        path: string;
        /** Registry key */
        name: string | null;
    } & ({
        dataType?: "REG_DWORD" | "REG_BINARY";
        value: string;
    } | {
        dataType: "REG_MULTI_SZ";
        value: string[];
    })[];

    links: {
        linkId: number;
        isFile: boolean;
        targetId: number;
        linkPath: string;
        targetPath: string;
    }[];

};