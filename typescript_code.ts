/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
/**
 *
 * @author 56130
 */
 class User {

	private  id: number ;
	private firstName: string;
	 private lastName: string;
	 private age: number;
	private profession: string;
	private children: User[];
  
	public constructor(id: number, firstName: string, lastName: string, age: number) {
  
	  this.id = id;
	  this.firstName = firstName;
	  this.lastName = lastName;
	  this.age = age;
	  this.profession = profession;
  
	}
  
	public getChildName(child: User): string {
	  if (!this.children.includes(child)) {
		new Error("Invalid argument!");
	  } else {
		let name: string | null = null;
		if (child.getFirstName() != null) {
		  name = child.getFirstName();
		}
		if (name == "Harry") {
		  name.replace("r", "p");
		}
		if (name != null || name.length > 0) {
		  name.concat(child.getLastName());
		}
	  }
	  return this.getChildName(child);
	}
  
	public getFirstName(): string {
	  return firstName;
	}
  
	public getLastName(): string {
	  return lastName;
	}
  }
  