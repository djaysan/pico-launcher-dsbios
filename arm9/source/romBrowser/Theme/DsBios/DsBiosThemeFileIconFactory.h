#pragma once
#include "../Material/MaterialThemeFileIconFactory.h"
#include "../Material/MaterialNdsFileIcon.h"

/// @brief Material's icons, except that a rom the launcher has no icon for gets
///        the CARTRIDGE glyph rather than the blank document one.
///
/// Anything that is not a DS rom - a .gba, .nes, .smc reached through a file
/// association - falls back to the generic file icon, which is a sheet of paper.
/// They are cartridges, and on a DS BIOS screen they should look like the DS
/// carts beside them. Box art cannot help here: a 32x32 icon has too few pixels
/// to carry a cover, a logo or a cartridge photo legibly - measured, all three
/// are mush - so the icon says what KIND of thing it is and the cover on the top
/// screen says which game.
class DsBiosThemeFileIconFactory : public MaterialThemeFileIconFactory
{
public:
    using MaterialThemeFileIconFactory::MaterialThemeFileIconFactory;

    std::unique_ptr<FileIcon> CreateGenericFileIcon(const TCHAR* name) const override
    {
        return CreateNdsFileIcon(name);
    }
};
