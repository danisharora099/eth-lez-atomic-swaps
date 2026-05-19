#pragma once

#include <string>

void swapDeliverySetRuntimeLogosAPI(void* api);

std::string swapDeliveryMessagingInit(const std::string& configJson);
std::string swapDeliveryMessagingShutdown();
std::string swapDeliveryMessagingStatus();
std::string swapDeliveryPublishOffer(const std::string& configJson);
std::string swapDeliveryFetchOffers();
std::string swapDeliveryEthAmountToWei(const std::string& ethAmount);

// Per-swap coordination on /atomic-swaps/1/swap-<hashlock>/json.
// Used by the maker to subscribe upon learning the hashlock (via on-chain
// ETH lock detection) and by the taker to publish a SwapAccept after locking
// ETH. The taker payload is supplementary to the on-chain flow — the
// orchestrator still drives the swap via watchers — but exercising the topic
// proves the M2 Delivery coordination path end-to-end.
std::string swapDeliverySubscribeSwap(const std::string& hashlockHex);
std::string swapDeliveryUnsubscribeSwap(const std::string& hashlockHex);
std::string swapDeliveryPublishSwapAccept(const std::string& configJson);
std::string swapDeliveryFetchSwapEvents(const std::string& hashlockHex);
