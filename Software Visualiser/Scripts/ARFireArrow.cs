using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;

public class ARFireArrow : MonoBehaviour
{
    [SerializeField] List<GameObject> prefabsToSpawn = new List<GameObject>();
    [SerializeField] List<GameObject> fakeprefabsToSpawn = new List<GameObject>();
    private Dictionary<string, GameObject> fakePrefabs;
    public GameObject SuperAbility;
    public float Strength
    {
        get { return strength; }
    }
    Vector3 cameraPosition;
    Quaternion cameraRotation;

    private Vector3 startpoint;
    private Vector3 endpoint;
    public float strength;
    public float maxDrawDistance;
    public LogicScript logic;
    private bool isReload = true;
    private bool hasFired = true;

    public bool HasFired
    {
        get {return hasFired;}
    }
    private bool canFire = true;
    public float FireStrength;

    public MQTTS_Test mqtt;

    void Start()
    {
        fakePrefabs = new Dictionary<string, GameObject>();
        SetupFakePrefabs();
        logic = GameObject.FindGameObjectWithTag("Logic").GetComponent<LogicScript>();
        mqtt = GameObject.FindGameObjectWithTag("Input").GetComponent<MQTTS_Test>();
    }

    private void SetupFakePrefabs()
    {
        foreach (var prefab in fakeprefabsToSpawn)
        {
            var obj = Instantiate(prefab, Camera.main.transform, false);
            obj.SetActive(false);
            obj.name = prefab.name;
            fakePrefabs.Add(obj.name, obj);
        }
    }

    // Update is called once per frame
    void Update()
    {
        if (logic.isTouch)
        {
            if (EventSystem.current.IsPointerOverGameObject() == false)
            {
                if (Input.GetMouseButtonDown(0))
                {
                    strength = 0f;
                    if (isReload)
                    {
                        ReloadObject(logic.ArrowType);
                        isReload = true;
                    } else
                    {
                        startpoint = Input.mousePosition;
                    }
                }

                if (Input.GetMouseButton(0))
                {
                    if (!isReload)
                    {
                        endpoint = Input.mousePosition;
                        float distance = startpoint[1] - endpoint[1];
                        float rawStrength = distance > 0 ? (distance > maxDrawDistance ? maxDrawDistance : distance) : 0;
                        strength = rawStrength / maxDrawDistance;
                    }
                }

                if (Input.GetMouseButtonUp(0))
                {
                    if (isReload)
                    {
                        isReload = false;
                    } else
                    {
                        PlaceObject(logic.ArrowType, strength);
                        isReload = true;
                    }
                }
            }
        } else {
            strength = mqtt.strength / 3f;

            if (mqtt.isFire == true)
            {
                if (!hasFired) // && canFire
                {
                    PlaceObject(logic.ArrowType, strength);
                    // canFire = false;
                    // StartCoroutine(SetCanFireAfterDelay());
                }
            }
        }

        cameraPosition = Camera.main.transform.position;
        cameraRotation = Camera.main.transform.rotation;

    }

    IEnumerator SetCanFireAfterDelay()
    {
        yield return new WaitForSecondsRealtime(0.5f);
        canFire = true;
    }

    public void PlaceObject(int ArrowType, float fireStrength)
    {
        FireStrength = fireStrength;
        var fakeObj = fakePrefabs[fakeprefabsToSpawn[ArrowType].name];
        fakeObj.SetActive(false);
        Instantiate(prefabsToSpawn[ArrowType], fakeObj.transform.position, fakeObj.transform.rotation);
        hasFired = true;
    }

    public void PlaceSuper()
    {
        Instantiate(SuperAbility, Camera.main.transform.position, Camera.main.transform.rotation);
    }

    public void ReloadObject(int ArrowType)
    {
        fakePrefabs[fakeprefabsToSpawn[ArrowType].name].SetActive(true);
        hasFired = false;
    }

    public void ClearFakeArrow(int ArrowType)
    {
        fakePrefabs[fakeprefabsToSpawn[ArrowType].name].SetActive(false);
        hasFired = true;
        isReload = true;
    }
}