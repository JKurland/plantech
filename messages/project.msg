// The project is an entity store, it contains the state for a 
// user's project

data ProjectEntityHandle {
    usize idx
}

request NewProjectEntity -> ProjectEntityHandle {
}

request RemoveProjectEntity -> void {
    ProjectEntityHandle entity
}

// Returns Nothing if the entity does not have this component, if this 
// component has not been registered or if this entity has been 
// removed, otherwise returns the component set with SetProjectEntityComponent
request GetProjectEntityComponent[ComponentT] -> option[ComponentT] {
    ProjectEntityHandle entity
}

// adds a component to an entity, or edits it if already set.
// Returns true if the component has been registered, false if not.
request SetProjectEntityComponent[ComponentT] -> bool {
    ProjectEntityHandle entity
    ComponentT component
}

// Removes component from an entity. After this call GetProjectEntityComponent
// will return Nothing. If component is not registered this does nothing.
request RemoveProjectEntityComponent[ComponentT] -> void {
    ProjectEntityHandle entity
}

request RegisterProjectComponent[ComponentT] -> void {
}
