-- NotifyConfig.lua
-- Key: animation file path (as used by UAnimSequenceBase::GetFilePath())
-- Value: table mapping NotifyName -> function(event)

-- ============================================================================
-- Audio Helper Functions (defined in Lua, not C++)
-- These use the generic PlaySound() function provided by C++
-- ============================================================================

function PlayFootstep()
    PlaySound("Data/Audio/FootSound.wav", 1.0)
end

-- function PlayJump()
--     PlaySound("Data/Audio/Shot.wav", 1.2)
-- end

-- function PlayLand()
--     PlaySound("Data/Audio/HitFireball.wav", 0.9)
-- end

-- ============================================================================
-- Notify Configuration
-- Maps animation sequences to notify handlers
-- ============================================================================

NotifyConfig = {
    ["Data/Animations/Standard Walk.fbx"] = {
        TEST = function(event)
            -- Now uses Lua-defined function
            PlayFootstep()
        end,

        WalkSound = function(event)
            PlayFootstep()
        end,

        Footstep = function(event)
            PlayFootstep()
        end,
    },

    -- Example: add another sequence with same notify names, different behavior
    ["Data/Animations/Run.fbx"] = {
        RunSound = function(event)
            -- Run animation plays louder footsteps
            PlaySound("Data/Audio/FootSound.wav", 1.3)
        end,
    },
}

