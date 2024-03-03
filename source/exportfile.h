#ifndef _H_exportfile
#define _H_exportfile

#ifndef _H_moaxtra
#include "moaxtra.h"
#endif

#ifndef _H_mmixscrp
#include "mmixscrp.h"
#endif

// {1E7A54B7-D4C6-45C2-8D57-E5BEE1CC25CD}
DEFINE_GUID(IID_IExportFile,
	0x1e7a54b7, 0xd4c6, 0x45c2, 0x8d, 0x57, 0xe5, 0xbe, 0xe1, 0xcc, 0x25, 0xcd);

enum {
	m_exportFileNew = 0, /* standard first entry */
	m_exportFileStatus,
	m_exportFileOut,
	m_tellExportFileMixerSaved,
	m_getExportFileLabelList,
	m_getExportFileAgentPropList,
	m_getExportFileDefaultPath,
	m_getExportFileDefaultLabel,
	m_getExportFileDefaultAgent,
	m_getExportFileDefaultOptions,
	m_getExportFileDisplayName,
	m_getExportFileTypePropList,
	m_getExportFileIconPropList,
	m_exportFile_Status,
	m_exportFile_Out,
	m_callTellExportFile,
	m_callGetExportFile,
	m_exportFileXXXX, /* standard last entry */
	m_getExportFileDefaultOptionsX = 16000 /* not in the Message Table - accessible to other Xtras only */
};

#endif