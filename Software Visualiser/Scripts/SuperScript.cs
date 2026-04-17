using System.Collections;
using Unity.VisualScripting;
using UnityEngine;

public class SuperScript : MonoBehaviour
{
    public Rigidbody myRigidbody;
    public float moveSpeed;
    private Vector3 movement;
    private float deadZone = 20f;
    public LogicScript logic;

    // Start is called once before the first execution of Update after the MonoBehaviour is created
    void Start()
    {
        logic = GameObject.FindGameObjectWithTag("Logic").GetComponent<LogicScript>();
        myRigidbody.AddRelativeForce(moveSpeed * Vector3.up, ForceMode.Force);
    }

    // Update is called once per frame
    void Update()
    {
        myRigidbody.MovePosition(transform.position + transform.up * moveSpeed * Time.deltaTime);
        if (transform.position[0] < -deadZone || transform.position[2] < -deadZone || transform.position[0] > deadZone || transform.position[2] > deadZone)
        {
            Destroy(transform.parent.gameObject);
        }
        if (logic.clearEnemy == true)
        {
            Destroy(transform.parent.gameObject);
        }
    }
}
