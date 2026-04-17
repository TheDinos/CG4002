using UnityEngine;

public class NewMonoBehaviourScript : MonoBehaviour
{
    public Rigidbody myRigidbody;
    public ARFireArrow aRFireArrow;
    public float moveSpeed;
    private float deadZone = -20f;
    public LogicScript logic;
    private int count = 0;

    // Start is called once before the first execution of Update after the MonoBehaviour is created
    void Start()
    {
        logic = GameObject.FindGameObjectWithTag("Logic").GetComponent<LogicScript>();
        aRFireArrow = FindFirstObjectByType<ARFireArrow>();
        float strength = aRFireArrow.FireStrength + (logic.isTouch ? 0 : 2);
        //Debug.Log(strength);
        myRigidbody.AddRelativeForce(strength * moveSpeed * Vector3.up, ForceMode.Impulse);
        myRigidbody.AddRelativeTorque(0.03f * Vector3.forward);
    }

    // Update is called once per frame
    void Update()
    {
        if (transform.position[1] < deadZone)
        {
            Destroy(transform.parent.gameObject);
        }
        if (logic.clearEnemy && !logic.isTutorialMode)
        {
            Destroy(transform.parent.gameObject);
        }
    }

    private void OnCollisionEnter(Collision collision)
    {   
        // if (collision.gameObject.layer == 7)
        // {
        //     Debug.Log("Arrow Hit Red Enemy");
        // } else if (collision.gameObject.layer == 9) 
        // {
        //     Debug.Log("Arrow Hit Blue Enemy");
        // } else 
        
        if (collision.gameObject.layer == 0)
        {
            //Debug.Log("Arrow Hit Wall");
            Destroy(transform.parent.gameObject);
        }
    }

    private void OnTriggerEnter(Collider collision)
    {
        if (collision.gameObject.layer == 7 || collision.gameObject.layer == 9 || collision.gameObject.layer == 11 || collision.gameObject.layer == 13)
        {
            count++;
        }
        if (count >= 3)
        {
            Destroy(transform.parent.gameObject);
        }
    }
}
