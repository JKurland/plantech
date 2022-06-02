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

// Returns Nothing if the entity does not have this component, otherwise
// returns the component set with SetProjectEntityComponent
request GetProjectEntityComponent[ComponentT] -> option[ComponentT] {
    ProjectEntityHandle entity
}

// adds a component to an entity, or edits it if already set
request SetProjectEntityComponent[ComponentT] -> void {
    ProjectEntityHandle entity
    ComponentT component
}

// Removes component from an entity. After this call GetProjectEntityComponent
// will return Nothing
request RemoveProjectEntityComponent[ComponentT] -> void {
    ProjectEntityHandle entity
}
