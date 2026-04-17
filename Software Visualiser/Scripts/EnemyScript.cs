using System.Collections;
using Unity.VisualScripting;
using UnityEngine;

public class PillarScript : MonoBehaviour
{
    [SerializeField] private Animator animator;
    public Rigidbody myRigidbody;
    private float moveSpeed = 0.5f;
    private float deadZone = -20;
    private Vector3 movement;
    public LogicScript logic;
    private bool isShot = false;
    private bool isAttack = false;
    // Start is called once before the first execution of Update after the MonoBehaviour is created
    void Start()
    {
        logic = GameObject.FindGameObjectWithTag("Logic").GetComponent<LogicScript>();
        if (UnityEngine.Random.Range(0, 100) > 90)
        moveSpeed *= UnityEngine.Random.Range(0.5f, 2f);
    }

    // Update is called once per frame
    void Update()
    {
        Vector3 cameraPosition = Camera.main.transform.position;
        if (!isShot)
        {
            movement = new Vector3(cameraPosition[0], transform.position[1], cameraPosition[2]);
            transform.position = Vector3.MoveTowards(transform.position, movement, moveSpeed * Time.deltaTime);
            transform.LookAt(movement);
        }
        if (transform.position[1] < deadZone)
        {
            Destroy(transform.parent.gameObject);
        }
        if (logic.clearEnemy == true)
        {
            Destroy(transform.parent.gameObject);
        }
    }

    // void FixedUpdate()
    // {
    //     moveCharacter(movement);
    // }

    // void moveCharacter(Vector3 direction)
    // {
    //     myRigidbody.linearVelocity = moveSpeed * Time.fixedDeltaTime * direction;
    //    // myRigidbody.AddTorque(movement);
    // }

    // private void OnTriggerEnter(Collider collision)
    // {   
    //     //Debug.Log("Collided");
    //     isAttack = true;
    //     animator.SetBool("isAttack", isAttack);
    //     StartCoroutine(DestroyPrefab());
    // }

    private void OnTriggerEnter(Collider collision)
    {   
        if (collision.gameObject.layer == gameObject.layer - 1)
        {
            //Debug.Log("Enemy Killed");
            isShot = true;
            animator.SetBool("isShot", isShot);
            StartCoroutine(KillPrefab(true));
        } else if (collision.gameObject.layer == 6 || collision.gameObject.layer == 8 || collision.gameObject.layer == 10 || collision.gameObject.layer == 12) {
            isShot = true;
            animator.SetBool("isShot", isShot);
            StartCoroutine(KillPrefab(false));
        } else
        {
            isAttack = true;
            animator.SetBool("isAttack", isAttack);
            StartCoroutine(DestroyPrefab());
        }
    }

    private void OnCollisionEnter(Collision collision)
    {
        if (collision.gameObject.layer == 14 && !isShot)
        {
            isShot = true;
            animator.SetBool("isShot", isShot);
            StartCoroutine(KillPrefab(true));
        }
    }

    IEnumerator KillPrefab(bool addScore)
    {
        myRigidbody.excludeLayers = MaskFromLayerNumbers(6, 7, 8, 9, 10, 11, 12, 13);
        if (addScore)
        {
            logic.AddScore(1);
        }
        yield return new WaitForSeconds(2f);
        Destroy(transform.parent.gameObject);
    }

    int MaskFromLayerNumbers(params int[] layers)
    {
        int mask = 0;
        foreach (int layer in layers)
        {
            mask |= (1 << layer);
        }
        return mask;
    }

    IEnumerator DestroyPrefab()
    {
        myRigidbody.excludeLayers = MaskFromLayerNumbers(6, 7, 8, 9, 10, 11, 12, 13);
        yield return new WaitForSeconds(0.75f);
        Destroy(transform.parent.gameObject);
        logic.TakeDamage(5f);
    }
}
