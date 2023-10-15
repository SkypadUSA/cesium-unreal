// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumWebMapTileServiceRasterOverlay.h"

#include "Cesium3DTilesSelection/WebMapTileServiceRasterOverlay.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumGeospatial/Projection.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"

#include "CesiumRuntime.h"

std::unique_ptr<Cesium3DTilesSelection::RasterOverlay>
UCesiumWebMapTileServiceRasterOverlay::CreateOverlay(
    const Cesium3DTilesSelection::RasterOverlayOptions& options) {
  Cesium3DTilesSelection::WebMapTileServiceRasterOverlayOptions wmtsOptions;
  if (!TileMatrixSetID.IsEmpty()) {
    wmtsOptions.tileMatrixSetID = TCHAR_TO_UTF8(*this->TileMatrixSetID);
  }
  if (!Style.IsEmpty()) {
    wmtsOptions.style = TCHAR_TO_UTF8(*this->Style);
  }
  if (!Layer.IsEmpty()) {
    wmtsOptions.layer = TCHAR_TO_UTF8(*this->Layer);
  }
  if (!Format.IsEmpty()) {
    wmtsOptions.format = TCHAR_TO_UTF8(*this->Format);
  }
  if (MaximumLevel > MinimumLevel && bSpecifyZoomLevels) {
    wmtsOptions.minimumLevel = MinimumLevel;
    wmtsOptions.maximumLevel = MaximumLevel;
  }

  CesiumGeospatial::Projection projection;
  if (this->UseWebMercatorProjection) {
    projection = CesiumGeospatial::WebMercatorProjection();
    wmtsOptions.projection = projection;
  } else {
    projection = CesiumGeospatial::GeographicProjection();
    wmtsOptions.projection = projection;
  }
  if (bSpecifyTilingScheme) {
    CesiumGeospatial::GlobeRectangle globeRectangle =
        CesiumGeospatial::GlobeRectangle::fromDegrees(West, South, East, North);
    CesiumGeometry::Rectangle coverageRectangle =
        CesiumGeospatial::projectRectangleSimple(projection, globeRectangle);
    wmtsOptions.coverageRectangle = coverageRectangle;
    wmtsOptions.tilingScheme = CesiumGeometry::QuadtreeTilingScheme(
        coverageRectangle,
        RootTilesX,
        RootTilesY);
  }

  if (!TileMatrixSetLabels.IsEmpty()) {
    std::vector<std::string> labels;
    for (const auto& label : this->TileMatrixSetLabels) {
      labels.emplace_back(TCHAR_TO_UTF8(*label));
    }
    wmtsOptions.tileMatrixLabels = labels;
  }
  return std::make_unique<
      Cesium3DTilesSelection::WebMapTileServiceRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      TCHAR_TO_UTF8(*this->Url),
      std::vector<CesiumAsync::IAssetAccessor::THeader>(),
      wmtsOptions,
      options);
}
