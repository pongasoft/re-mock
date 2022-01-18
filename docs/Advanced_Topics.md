Advanced Topics
---------------

### Resources

Rack extensions have access to resources: patches, samples and blobs. `re-mock` handles all resources and goes beyond by allowing to mock resources (replace the ones that the device would load by "simpler" ones) as well as simulating load issues (slow / errors).

> #### Note
> Mocking a resource is the ability to tell the framework to replace a resource that the device would normally load, by a different one (usually smaller/simpler) for testing purposes. The way mocking resources is configured is through the `DeviceConfig` api.

#### Mocking Patches

The file `info.lua` can define a `default_patch` and this can easily be changed by using `DeviceConfig::default_patch()` api.
```cpp
auto c = DeviceConfig<Device>::fromJBoxExport(RE_CMAKE_PROJECT_DIR)
  .default_patch("/Public/Init.repatch"); // change the default patch defined in info.lua
```
 
In order to replace an existing patch by a different one, the api `patch_string`, `patch_file` or `patch_data` is used:
```cpp
auto c = DeviceConfig<Device>::fromJBoxExport(RE_CMAKE_PROJECT_DIR)
  .patch_file("/Public/Bass.repatch", "<path to another patch file>")
  .patch_string("/Public/Kick.repatch", R"(<?xml version="1.0"?><JukeboxPatch version..."))
  .patch_data("/Public/Snare.repatch", resource::Patch{}.number("/custom_property/gain", 0.7));
```

> #### Note
> The default patch defined in `info.lua` is the only patch that is loaded without user intervention as a result. The main API to load a patch is on the device itself to simulate user interaction:
> ```cpp
> tester.device().loadPatch("/Public/Bass.repatch"); // using a built-in patch
> tester.device().loadPatch(resource::File{"<path to local patch>"}); // using a patch on the file system
> tester.device().loadPatch(resource::Patch{}.number("/custom_property/gain", 0.7)); // using a patch object
> ```

Check the test [TestPatch.cpp](../test/cpp/re/mock/TestPatch.cpp) for many examples on how to use these APIs.

#### Mocking blobs

Blobs are opaque arrays of data internal to the Rack Extension. The only way to load a blob is using the `jbox.load_blob_async()` (or `jbox.make_empty_blob()`) api in a realtime controller binding (lua).

In order to replace an existing blob by a different one, the api `blob_file` or `blob_data` is used:

```cpp
auto c = DeviceConfig<Device>::fromJBoxExport(RE_CMAKE_PROJECT_DIR)
  .blob_data("/Private/blob.data", {0, 1, 2, 3})
  .blob_file("/Private/blob.file", "<path to a file>");
```

These mock blobs are used when the lua code invokes `jbox.load_blob_async('/Private/blob.data')` (resp. `jbox.load_blob_async('/Private/blob.file')`) and they replace the ones built into the device (`Resources` folder).

> #### Note
> Since there is no user interaction to load a blob, there is no api on the device itself to load one: it needs to be loaded in a realtime controller binding (lua).

#### Mocking blob slow/failure loading

The `DeviceConfig::resource_loading_context()` api is used to simulate a slow or failed status when loading a resource (in this instance a blob):

```cpp
auto c = DeviceConfig<Device>::fromJBoxExport(RE_CMAKE_PROJECT_DIR)
  .blob_data("/Private/blob.data", {0, 1, 2, 3})
  .blob_file("/Private/blob.file", "<path to a file>")
  .resource_loading_context("/Private/blob.data", resource::LoadingContext{}.status(resource::LoadStatus::kMissing))
  .resource_loading_context("/Private/blob.file", resource::LoadingContext{}.status(resource::LoadStatus::kPartiallyResident).resident_size(100));
```

This example attaches a `resource::LoadingContext` to both blobs, and it is used when the blob is loaded. The `jbox.get_blob_info()` lua api returns the status of the load operation and if a loading context is provided, it is used instead.

The `/Private/blob.data` blob has been marked missing and as a result will not load and report a status of missing in lua (`jbox.get_blob_info()`)

The `/Private/blob.file` blob has been marked partially resident with 100 bytes of data available. In order to make additional bytes available, the `loadMoreBlob` api is used on the device like so:

```cpp
tester.device().loadMoreBlob("/custom_properties/prop_blob", 50); // to load an additional 50 bytes
tester.device().loadMoreBlob("/custom_properties/prop_blob"); // to load all of it
```

> #### Note
> The `loadMoreBlob` api uses the property path (the one that the lua code loads the blob into) **not** the resource path of the blob:
> ```lua
> global_rtc = {
>   on_blob = function(source_property_path, new_value)
>     local new_no = jbox.load_blob_async("/Private/blob.data")
>     jbox.store_property("/custom_properties/prop_blob", new_no)
>     -- ...
>   end
> } 
> ```

Check the test [TestRackExtension.cpp / RealtimeController_Blob](../test/cpp/re/mock/TestRackExtension.cpp) unit test for examples on how to use these APIs.

#### Mocking samples

Rack Extensions supports 2 kinds of samples: built-in samples and user provided samples. `re-mock` supports both kinds.

In order to replace an existing sample by a different one, the api `sample_file` or `sample_data` is used:

```cpp
auto c = DeviceConfig<Device>::fromJBoxExport(RE_CMAKE_PROJECT_DIR)
  .sample_data("/Private/sample.default", resource::Sample{}.sample_rate(44100).channels(1).data({0,0.5,0.25,1}))
  .sample_file("/Private/mono_sample.file", "<path to a file>"))
```

These mock samples are used when the lua code invokes `jbox.load_sample_async('/Private/sample.default')` (resp. `jbox.load_sample_async('/Private/mono_sample.file')`) and they replace the ones built into the device (`Resources` folder).

To deal with user samples, which are user provided, there is a set of apis directly on the device

```cpp
tester.device().loadUserSampleAsync("/user_samples/1/item", "/Private/stereo_sample.data");
tester.device().loadUserSampleAsync(1, "/Private/stereo_sample.data"); // same but simpler
```

> #### Note
> The second parameter is a `resourcePath` and it can be a path pointing in the device (like in this example), or directly a path on the filesystem. 

#### Mocking samples slow/failure loading

The `DeviceConfig::resource_loading_context()` api is used to simulate a slow or failed status when loading a resource (in this instance a sample):

```cpp
auto c = DeviceConfig<Device>::fromJBoxExport(RE_CMAKE_PROJECT_DIR)
  .sample_data("/Private/sample.default", resource::Sample{}.sample_rate(44100).channels(1).data({0,0.5,0.25,1}))
  .sample_file("/Private/mono_sample.file", "<path to a file>"))
  .resource_loading_context("/Private/sample.default", resource::LoadingContext{}.status(resource::LoadStatus::kMissing))
  .resource_loading_context("/Private/mono_sample.file", resource::LoadingContext{}.status(resource::LoadStatus::kPartiallyResident).resident_size(100));
```

This example attaches a `resource::LoadingContext` to both samples, and it is used when the sample is loaded. The `jbox.get_sample_info()` lua api returns the status of the load operation and if a loading context is provided, it is used instead.

The `/Private/sample.default` sample has been marked missing and as a result will not load and report a status of missing in lua (`jbox.get_sample_info()`)

The `/Private/mono_sample.file` sample has been marked partially resident with 100 bytes of data available. In order to make additional bytes available, the `loadMoreSample` api is used on the device like so:

```cpp
tester.device().loadMoreSample("/user_samples/1/item", 50); // to load an additional 50 bytes
tester.device().loadMoreSample("/user_samples/1/item"); // to load all of it
```

The previously described api (`tester.device().loadUserSampleAsync()`) also takes an additional optional parameter providing a `resource::LoadingContext` to achieve the same kind of desired result.

Check the test [TestRackExtension.cpp / RealtimeController_UserSample](../test/cpp/re/mock/TestRackExtension.cpp) unit test for examples on how to use these APIs.

#### Mocking slow/failure loading

As shown in the case of samples and blobs, the `DeviceConfig` offers an API to associate a `resource::LoadingContext` to a resource path. A similar API exists on the device itself in order to be able to handle cases after the device has been instantiated:

```cpp
tester.device().setResourceLoadingContext("/Private/sample.default", resource::LoadingContext{}.status(resource::LoadStatus::kMissing));
tester.device().clearResourceLoadingContext("/Private/sample.default"); // to remove it
```