using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UIElements;
using UnityEngine.XR.ARFoundation;
using UnityEngine.XR.ARFoundation.VisualScripting;
using UnityEngine.XR.ARSubsystems;

public class ImageTracker : MonoBehaviour
{
    // Prefabs to spawn
    [SerializeField] List<GameObject> prefabsToSpawn = new List<GameObject>();
    private float spawnDelay = 6f;
    private float baseSpawnDelay;
    private Dictionary<string, float> delay;
    private Dictionary<string, bool> canSpawn;
    private Dictionary<GameObject, Transform> prefabPosition;
    private int imagesDetected;
    public int ImagesDetected
    {
        get { return imagesDetected; }
    }
    private int numOfPrefabs;
    public int NumOfPrefabs
    {
        get { return numOfPrefabs; }
    }
    public LogicScript logic;

    // ARTrackedImageManager reference
    private ARTrackedImageManager trackedImageManager;

    // Initialisation and references assigning
    private void Start() {
        trackedImageManager = GetComponent<ARTrackedImageManager>();
        if (trackedImageManager == null) return;
        trackedImageManager.trackablesChanged.AddListener(OnImagesTrackedChanged);
        canSpawn = new Dictionary<string, bool>();
        delay = new Dictionary<string, float>();
        prefabPosition = new Dictionary<GameObject, Transform>();
        foreach (var prefab in prefabsToSpawn)
        {
            canSpawn.Add(prefab.name, true);
            delay.Add(prefab.name, UnityEngine.Random.Range(0f, spawnDelay));
        }
        numOfPrefabs = prefabsToSpawn.Count;
        logic = GameObject.FindGameObjectWithTag("Logic").GetComponent<LogicScript>();
        baseSpawnDelay = spawnDelay;
    }

    private void OnDestroy() {
        trackedImageManager.trackablesChanged.RemoveListener(OnImagesTrackedChanged);
    }

    // Update tracked images and prefabs
    private void OnImagesTrackedChanged(ARTrackablesChangedEventArgs<ARTrackedImage> eventArgs) {
        foreach (var trackedImage in eventArgs.added)
        {
            addTrackedImages(trackedImage);
        }
        foreach (var trackedImage in eventArgs.updated)
        {
            updateTrackedImages(trackedImage);
        }
        // foreach (var trackedImage in eventArgs.removed)
        // {
        //     updateTrackedImages(trackedImage.Value);
        // }
        
    }

    private void updateTrackedImages(ARTrackedImage trackedImage) {
        // if (trackedImage == null) return;
        // if (trackedImage.trackingState is TrackingState.Limited or TrackingState.None) {
        //     Debug.Log(trackedImage.referenceImage.name + " Lost");
        //     return;
        // }

        foreach (var arPrefab in prefabsToSpawn)
        {
            if (trackedImage.referenceImage.name == arPrefab.name && canSpawn[arPrefab.name] == true)
            {
                // Instantiate(arPrefab, trackedImage.transform);
                // canSpawn[arPrefab.name] = false;
                // StartCoroutine(SetCanSpawnToTrueWithDelay(arPrefab.name));

                prefabPosition[arPrefab] = trackedImage.transform;

                // if (prefabPosition.Count != prefabsToSpawn.Count)
                // {
                //     prefabPosition.Add(arPrefab, trackedImage.transform);
                // }
            }      
        }
        

        // arObjects[trackedImage.referenceImage.name].gameObject.SetActive(true);
        // arObjects[trackedImage.referenceImage.name].transform.position = trackedImage.transform.position;
        // arObjects[trackedImage.referenceImage.name].transform.rotation = trackedImage.transform.rotation;

    }

    private void addTrackedImages(ARTrackedImage trackedImage) {
        // if (trackedImage == null) return;
        // if (trackedImage.trackingState is TrackingState.Limited or TrackingState.None) {
        //     Debug.Log(trackedImage.referenceImage.name + " Lost");
        //     return;
        // }

        foreach (var arPrefab in prefabsToSpawn)
        {
            if (trackedImage.referenceImage.name == arPrefab.name && canSpawn[arPrefab.name] == true)
            {
                // Instantiate(arPrefab, trackedImage.transform);
                // canSpawn[arPrefab.name] = false;
                // StartCoroutine(SetCanSpawnToTrueWithDelay(arPrefab.name));

                prefabPosition.Add(arPrefab, trackedImage.transform);
            }      
        }
        

        // arObjects[trackedImage.referenceImage.name].gameObject.SetActive(true);
        // arObjects[trackedImage.referenceImage.name].transform.position = trackedImage.transform.position;
        // arObjects[trackedImage.referenceImage.name].transform.rotation = trackedImage.transform.rotation;

    }

    void Update()
    {
        foreach (var arPrefab in prefabPosition)
        {    
            if (canSpawn[arPrefab.Key.name] == true && logic.clearEnemy == false)
            {
                canSpawn[arPrefab.Key.name] = false;
                StartCoroutine(SetCanSpawnToTrueWithDelay(arPrefab.Key.name, arPrefab));
            }

        }
        imagesDetected = prefabPosition.Count;
    }

    public void clearPositions()
    {
        prefabPosition = new Dictionary<GameObject, Transform>();
        ResetCoroutines();
    }

    public void ResetCoroutines()
    {
        StopAllCoroutines();
        foreach (var prefab in prefabsToSpawn)
        {
            canSpawn[prefab.name] = true;
            delay[prefab.name] = UnityEngine.Random.Range(0f, baseSpawnDelay);
        }
    }

    IEnumerator SetCanSpawnToTrueWithDelay(string prefabName, KeyValuePair<GameObject, Transform> prefab)
    {
        yield return new WaitForSeconds(delay[prefab.Key.name]);
        Instantiate(prefab.Key, prefab.Value);
        delay[prefab.Key.name] = UnityEngine.Random.Range(3f, baseSpawnDelay);
        canSpawn[prefab.Key.name] = true;
    }


    // private ARTrackedImageManager trackedImages;
    // public GameObject[] ArPrefabs;

    // List<GameObject> ARObjects = new List<GameObject>();

    
    // void Awake()
    // {
    //     trackedImages = GetComponent<ARTrackedImageManager>();
    // }

    // void OnEnable()
    // {
    //     trackedImages.trackedImagesChanged += OnTrackedImagesChanged;
    // }

    // void OnDisable()
    // {
    //     trackedImages.trackedImagesChanged -= OnTrackedImagesChanged;
    // }


    // // Event Handler
    // private void OnTrackedImagesChanged(ARTrackedImagesChangedEventArgs eventArgs)
    // {
    //     //Create object based on image tracked
    //     foreach (var trackedImage in eventArgs.added)
    //     {
    //         foreach (var arPrefab in ArPrefabs)
    //         {
    //             if(trackedImage.referenceImage.name == arPrefab.name)
    //             {
    //                 var newPrefab = Instantiate(arPrefab, trackedImage.transform);
    //                 ARObjects.Add(newPrefab);
    //             }
    //         }
    //     }
        
    //     //Update tracking position
    //     foreach (var trackedImage in eventArgs.updated)
    //     {
    //         foreach (var gameObject in ARObjects)
    //         {
    //             if(gameObject.name == trackedImage.name)
    //             {
    //                 gameObject.SetActive(trackedImage.trackingState == TrackingState.Tracking);
    //             }
    //         }
    //     }
        
    // }
}