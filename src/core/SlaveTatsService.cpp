#include "core/SlaveTatsService.h"

#include <cmath>
#include <optional>
#include <utility>

namespace stui::core {
namespace {

bool isValidArea(TattooArea area) noexcept {
    switch (area) {
    case TattooArea::body:
    case TattooArea::face:
    case TattooArea::hands:
    case TattooArea::feet:
        return true;
    }

    return false;
}

std::optional<ServiceError> validateLoadoutValue(const TattooLoadout& loadout) {
    const auto validation = validateTattooLoadout(loadout);
    if (validation) return std::nullopt;

    switch (validation.error()) {
    case TattooLoadoutValidationError::missingName:
    case TattooLoadoutValidationError::missingIdentity:
        return ServiceError{ServiceErrorCode::tattooNotFound, "Tattoo identity is required"};
    case TattooLoadoutValidationError::invalidArea:
        return ServiceError{ServiceErrorCode::invalidArea, "Invalid tattoo area"};
    case TattooLoadoutValidationError::invalidSlot:
        return ServiceError{ServiceErrorCode::invalidSlot, "Invalid tattoo slot"};
    case TattooLoadoutValidationError::duplicatePosition:
        return ServiceError{ServiceErrorCode::invalidSlot,
            "Tattoo loadout contains duplicate area and slot entries"};
    case TattooLoadoutValidationError::invalidColor:
    case TattooLoadoutValidationError::invalidAppearance:
        return ServiceError{ServiceErrorCode::updateFailed,
            "Tattoo loadout appearance values are invalid"};
    }
    return ServiceError{ServiceErrorCode::updateFailed, "Tattoo loadout is invalid"};
}

}  // namespace

SlaveTatsService::SlaveTatsService(ITattooRuntime& runtime) noexcept :
    m_runtime(runtime) {}

TattooQueryResult SlaveTatsService::queryAvailable(std::string_view domain) {
    if (!m_runtime.apiAvailable()) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::slaveTatsUnavailable,
            "SlaveTatsNG not available",
        });
    }

    if (!m_runtime.jContainersReady()) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::jContainersUnavailable,
            "JContainers not ready",
        });
    }

    return m_runtime.queryAvailable(domain);
}

TattooSlotsResult SlaveTatsService::querySlots(std::uint32_t actorFormId, TattooArea area) {
    if (!m_runtime.apiAvailable()) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::slaveTatsUnavailable,
            "SlaveTatsNG not available",
        });
    }

    if (!m_runtime.jContainersReady()) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::jContainersUnavailable,
            "JContainers not ready",
        });
    }

    if (actorFormId == 0) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::actorNotFound,
            "未找到角色",
        });
    }

    if (!isValidArea(area)) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::invalidArea,
            "Invalid tattoo area",
        });
    }

    return m_runtime.querySlots(actorFormId, area);
}

CatalogPreviewBeginResult SlaveTatsService::beginCatalogPreview(
    const CatalogPreviewBeginRequest& request) {
    if (!m_runtime.apiAvailable()) return std::unexpected(ServiceError{
        ServiceErrorCode::slaveTatsUnavailable, "SlaveTatsNG not available"});
    if (!m_runtime.jContainersReady()) return std::unexpected(ServiceError{
        ServiceErrorCode::jContainersUnavailable, "JContainers not ready"});
    if (!request.actorFormId) return std::unexpected(ServiceError{
        ServiceErrorCode::actorNotFound, "未找到角色"});
    if (!isValidArea(request.area)) return std::unexpected(ServiceError{
        ServiceErrorCode::invalidArea, "Invalid tattoo area"});
    if (request.slot < 0) return std::unexpected(ServiceError{
        ServiceErrorCode::invalidSlot, "Invalid tattoo slot"});
    return m_runtime.beginCatalogPreview(request);
}

CatalogPreviewPaintResult SlaveTatsService::paintCatalogPreview(
    const CatalogPreviewPaintRequest& request) {
    if (!request.sessionId) return std::unexpected(ServiceError{
        ServiceErrorCode::previewUnavailable, "目录预览会话不可用"});
    if (!request.context.actorFormId) return std::unexpected(ServiceError{
        ServiceErrorCode::actorNotFound, "未找到角色"});
    if (!isValidArea(request.context.area)) return std::unexpected(ServiceError{
        ServiceErrorCode::invalidArea, "Invalid tattoo area"});
    if (request.context.slot < 0) return std::unexpected(ServiceError{
        ServiceErrorCode::invalidSlot, "Invalid tattoo slot"});
    if (request.candidate &&
        (request.candidate->section.empty() || request.candidate->name.empty())) {
        return std::unexpected(ServiceError{ServiceErrorCode::tattooNotFound,
            "Tattoo section and name are required"});
    }
    if (request.candidate &&
        (request.candidate->color < 0 || request.candidate->color > 0xFFFFFF ||
        request.candidate->glow < 0 || request.candidate->glow > 0xFFFFFF ||
        !std::isfinite(request.candidate->alpha) || request.candidate->alpha < 0.0F ||
        request.candidate->alpha > 1.0F ||
        !std::isfinite(request.candidate->glossiness) || request.candidate->glossiness < 0.0F ||
        !std::isfinite(request.candidate->specularStrength) ||
        request.candidate->specularStrength < 0.0F ||
        !std::isfinite(request.candidate->emissiveMult) ||
        request.candidate->emissiveMult < 0.0F)) {
        return std::unexpected(ServiceError{ServiceErrorCode::previewFailed,
            "无效目录预览外观"});
    }
    return m_runtime.paintCatalogPreview(request);
}

CatalogPreviewRestoreResult SlaveTatsService::restoreCatalogPreview(
    const CatalogPreviewRestoreRequest& request) {
    if (!request.sessionId) return std::unexpected(ServiceError{
        ServiceErrorCode::previewUnavailable, "目录预览会话不可用"});
    if (!request.context.actorFormId) return std::unexpected(ServiceError{
        ServiceErrorCode::actorNotFound, "未找到角色"});
    if (!isValidArea(request.context.area)) return std::unexpected(ServiceError{
        ServiceErrorCode::invalidArea, "Invalid tattoo area"});
    if (request.context.slot < 0) return std::unexpected(ServiceError{
        ServiceErrorCode::invalidSlot, "Invalid tattoo slot"});
    return m_runtime.restoreCatalogPreview(request);
}

CatalogPreviewCommitResult SlaveTatsService::commitCatalogPreview(
    const CatalogPreviewCommitRequest& request) {
    if (!m_runtime.apiAvailable()) return std::unexpected(ServiceError{
        ServiceErrorCode::slaveTatsUnavailable, "SlaveTatsNG not available"});
    if (!m_runtime.jContainersReady()) return std::unexpected(ServiceError{
        ServiceErrorCode::jContainersUnavailable, "JContainers not ready"});
    if (!request.sessionId) return std::unexpected(ServiceError{
        ServiceErrorCode::previewUnavailable, "目录预览会话不可用"});
    if (!request.context.actorFormId ||
        request.apply.actorFormId != request.context.actorFormId) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::actorNotFound, "目录预览角色与保存不匹配"});
    }
    if (!isValidArea(request.context.area) || request.apply.area != request.context.area) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::invalidArea, "目录预览区域与保存不匹配"});
    }
    if (request.context.slot < 0 || request.apply.slot != request.context.slot) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::invalidSlot, "目录预览槽位与保存不匹配"});
    }
    if (request.apply.section.empty() || request.apply.name.empty()) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::tattooNotFound, "Tattoo section and name are required"});
    }
    if (request.apply.mode == ApplyTattooMode::applyAndSynchronize) {
        if (request.apply.color < 0 || request.apply.color > 0xFFFFFF ||
            !std::isfinite(request.apply.alpha) || request.apply.alpha < 0.0F ||
            request.apply.alpha > 1.0F) {
            return std::unexpected(ServiceError{
                ServiceErrorCode::applyFailed, "无效目录预览外观"});
        }
        if (request.apply.appearance) {
            const auto& a = *request.apply.appearance;
            const auto invalidStrength = [](const std::optional<float>& value) {
                return value && (!std::isfinite(*value) || *value < 0.0F);
            };
            if ((a.glow && (*a.glow < 0 || *a.glow > 0xFFFFFF)) ||
                invalidStrength(a.glossiness) || invalidStrength(a.specularStrength) ||
                invalidStrength(a.emissiveMult)) {
                return std::unexpected(ServiceError{
                    ServiceErrorCode::applyFailed, "无效高级目录预览外观"});
            }
        }
    }
    return m_runtime.commitCatalogPreview(request);
}

ApplyTattooResult SlaveTatsService::applyToSlot(const ApplyTattooRequest& request) {
    if (!m_runtime.apiAvailable()) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::slaveTatsUnavailable,
            "SlaveTatsNG not available",
        });
    }

    if (!m_runtime.jContainersReady()) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::jContainersUnavailable,
            "JContainers not ready",
        });
    }

    if (request.actorFormId == 0) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::actorNotFound,
            "未找到角色",
        });
    }

    if (!isValidArea(request.area)) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::invalidArea,
            "Invalid tattoo area",
        });
    }

    if (request.mode == ApplyTattooMode::synchronizeOnly) {
        return m_runtime.applyToSlot(request);
    }

    if (request.slot < 0) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::invalidSlot,
            "Invalid tattoo slot",
        });
    }

    if (request.section.empty() || request.name.empty()) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::tattooNotFound,
            "Tattoo section and name are required",
        });
    }

    if (!(request.alpha >= 0.0F && request.alpha <= 1.0F)) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::applyFailed,
            "纹身透明度须在0到1之间",
        });
    }

    if (request.appearance) {
        const auto& a = *request.appearance;
        const auto invalidStrength = [](const std::optional<float>& value) {
            return value && (!std::isfinite(*value) || *value < 0.0F);
        };
        if ((a.glow && (*a.glow < 0 || *a.glow > 0xFFFFFF)) ||
            invalidStrength(a.glossiness) || invalidStrength(a.specularStrength) ||
            invalidStrength(a.emissiveMult)) {
            return std::unexpected(ServiceError{
                ServiceErrorCode::applyFailed, "无效高级纹身外观"});
        }
    }
    return m_runtime.applyToSlot(request);
}

RemoveTattooResult SlaveTatsService::removeFromSlot(const RemoveTattooRequest& request) {
    if (!m_runtime.apiAvailable()) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::slaveTatsUnavailable,
            "SlaveTatsNG not available",
        });
    }

    if (!m_runtime.jContainersReady()) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::jContainersUnavailable,
            "JContainers not ready",
        });
    }

    if (request.actorFormId == 0) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::actorNotFound,
            "未找到角色",
        });
    }

    if (!isValidArea(request.area)) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::invalidArea,
            "Invalid tattoo area",
        });
    }

    if (request.slot < 0) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::invalidSlot,
            "Invalid tattoo slot",
        });
    }

    return m_runtime.removeFromSlot(request);
}

UpdateTattooAppearanceResult SlaveTatsService::updateAppearance(
    const UpdateTattooAppearanceRequest& request) {
    if (!m_runtime.apiAvailable()) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::slaveTatsUnavailable,
            "SlaveTatsNG not available",
        });
    }

    if (!m_runtime.jContainersReady()) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::jContainersUnavailable,
            "JContainers not ready",
        });
    }

    if (request.actorFormId == 0) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::actorNotFound,
            "未找到角色",
        });
    }

    if (request.mode == UpdateTattooAppearanceMode::updateAndSynchronize &&
        request.runtimeHandle == 0) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::staleTattooHandle,
            "纹身手柄无效；请刷新槽位快照后重试",
        });
    }

    if (request.mode == UpdateTattooAppearanceMode::updateAndSynchronize) {
        if (request.color < 0 || request.color > 0xFFFFFF) {
            return std::unexpected(ServiceError{
                ServiceErrorCode::updateFailed,
                "纹身颜色须在0到0xFFFFFF之间",
            });
        }

        if (!std::isfinite(request.alpha) || request.alpha < 0.0F || request.alpha > 1.0F) {
            return std::unexpected(ServiceError{
                ServiceErrorCode::updateFailed,
                "纹身透明度须在0到1之间",
            });
        }

        if (request.glow < 0 || request.glow > 0xFFFFFF) {
            return std::unexpected(ServiceError{
                ServiceErrorCode::updateFailed,
                "纹身发光须在0到0xFFFFFF之间",
            });
        }

        if (!std::isfinite(request.glossiness) || request.glossiness < 0.0F) {
            return std::unexpected(ServiceError{
                ServiceErrorCode::updateFailed,
                "纹身光泽须为有限非负数",
            });
        }

        if (!std::isfinite(request.specularStrength) || request.specularStrength < 0.0F) {
            return std::unexpected(ServiceError{
                ServiceErrorCode::updateFailed,
                "纹身高光强度须为有限非负数",
            });
        }

        if (!std::isfinite(request.emissiveMult) || request.emissiveMult < 0.0F) {
            return std::unexpected(ServiceError{
                ServiceErrorCode::updateFailed,
                "纹身自发光倍数须为有限非负数",
            });
        }
    }

    return m_runtime.updateAppearance(request);
}

SetTattooLockedResult SlaveTatsService::setTattooLocked(
    const SetTattooLockedRequest& request) {
    if (!m_runtime.apiAvailable()) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::slaveTatsUnavailable,
            "SlaveTatsNG not available",
        });
    }

    if (!m_runtime.jContainersReady()) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::jContainersUnavailable,
            "JContainers not ready",
        });
    }

    if (request.actorFormId == 0) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::actorNotFound,
            "未找到角色",
        });
    }

    if (request.runtimeHandle == 0) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::staleTattooHandle,
            "纹身手柄无效；请刷新槽位快照后重试",
        });
    }

    return m_runtime.setTattooLocked(request);
}

CaptureTattooLoadoutResult SlaveTatsService::captureLoadout(
    const CaptureTattooLoadoutRequest& request) {
    if (!m_runtime.apiAvailable()) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::slaveTatsUnavailable, "SlaveTatsNG not available"});
    }
    if (!m_runtime.jContainersReady()) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::jContainersUnavailable, "JContainers not ready"});
    }
    if (request.actorFormId == 0) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::actorNotFound, "未找到角色"});
    }
    return m_runtime.captureLoadout(request);
}

PreviewTattooLoadoutResult SlaveTatsService::previewLoadout(
    const PreviewTattooLoadoutRequest& request) {
    if (!m_runtime.apiAvailable()) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::slaveTatsUnavailable, "SlaveTatsNG not available"});
    }
    if (!m_runtime.jContainersReady()) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::jContainersUnavailable, "JContainers not ready"});
    }
    if (request.actorFormId == 0) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::actorNotFound, "未找到角色"});
    }
    if (auto invalid = validateLoadoutValue(request.loadout)) {
        return std::unexpected(std::move(*invalid));
    }
    return m_runtime.previewLoadout(request);
}

RestoreTattooLoadoutResult SlaveTatsService::restoreLoadout(
    const RestoreTattooLoadoutRequest& request) {
    if (!m_runtime.apiAvailable()) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::slaveTatsUnavailable, "SlaveTatsNG not available"});
    }
    if (!m_runtime.jContainersReady()) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::jContainersUnavailable, "JContainers not ready"});
    }
    if (request.actorFormId == 0) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::actorNotFound, "未找到角色"});
    }
    if (auto invalid = validateLoadoutValue(request.loadout)) {
        return std::unexpected(std::move(*invalid));
    }
    return m_runtime.restoreLoadout(request);
}

RestoreTattooLoadoutResult SlaveTatsService::retryLoadoutSynchronization(
    const RetryTattooLoadoutSynchronizationRequest& request) {
    if (!m_runtime.apiAvailable()) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::slaveTatsUnavailable, "SlaveTatsNG not available"});
    }
    if (!m_runtime.jContainersReady()) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::jContainersUnavailable, "JContainers not ready"});
    }
    if (request.actorFormId == 0) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::actorNotFound, "未找到角色"});
    }
    if (request.retryId == 0) {
        return std::unexpected(ServiceError{
            ServiceErrorCode::synchronizeFailed, "纹身配置重试无效"});
    }
    return m_runtime.retryLoadoutSynchronization(request);
}

}  // namespace stui::core
